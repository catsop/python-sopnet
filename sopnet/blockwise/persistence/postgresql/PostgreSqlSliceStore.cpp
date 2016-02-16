#include "config.h"
#ifdef HAVE_PostgreSQL

#include "PostgreSqlUtils.h"
#include <boost/tokenizer.hpp>
#include <boost/timer/timer.hpp>
#include <fstream>
#include <sys/stat.h>
#include <vigra/impex.hxx>
#include <imageprocessing/ConnectedComponent.h>
#include <imageprocessing/Image.h>
#include <slices/ComponentTreeConverter.h>
#include <util/ProgramOptions.h>
#include <util/httpclient.h>
#include <util/box.hpp>
#include <util/point.hpp>
#include "PostgreSqlSliceStore.h"
#include <slices/SliceHash.h>

logger::LogChannel postgresqlslicestorelog("postgresqlslicestorelog", "[PostgreSqlSliceStore] ");

PostgreSqlSliceStore::PostgreSqlSliceStore(
        const ProjectConfiguration& config) : _config(config)
{
	_pgConnection = PostgreSqlUtils::getConnection(
			_config.getPostgreSqlHost(),
			_config.getPostgreSqlPort(),
			_config.getPostgreSqlDatabase(),
			_config.getPostgreSqlUser(),
			_config.getPostgreSqlPassword());
	std::ostringstream q;
	q << "SET search_path TO segstack_"
	  << _config.getCatmaidStack(Membrane).segmentationId
	  << ",public;";
	PQsendQuery(_pgConnection, q.str().c_str());
}

PostgreSqlSliceStore::~PostgreSqlSliceStore() {

	if (_pgConnection != 0) {
		PostgreSqlUtils::waitForAsyncQuery(_pgConnection);
		PQfinish(_pgConnection);
	}
}

void
PostgreSqlSliceStore::associateSlicesToBlock(const Slices& slices, const Block& block, bool doneWithBlock) {

	boost::timer::cpu_timer queryTimer;

	std::string blockQuery = PostgreSqlUtils::createBlockIdQuery(block);

	std::ostringstream q;

	if (slices.size() == 0) {
		if (doneWithBlock) {
			q << "UPDATE block SET slices_flag = TRUE WHERE id = (" << blockQuery << ");";
			std::string query = q.str();
			PostgreSqlUtils::waitForAsyncQuery(_pgConnection);
			int asyncStatus = PQsendQuery(_pgConnection, query.c_str());
			if (0 == asyncStatus) {
				LOG_ERROR(postgresqlslicestorelog) << "PQsendQuery returned 0" << std::endl;
				LOG_ERROR(postgresqlslicestorelog) << "The used query was: " << query <<
					std::endl;
				UTIL_THROW_EXCEPTION(PostgreSqlException, "PQsendQuery returned 0");
			}
		}

		return;
	}

	std::ostringstream tmpTableStream;
	tmpTableStream << "slice_tmp_" << block.x() << "_" << block.y() << "_" << block.z();
	const std::string tmpTable = tmpTableStream.str();

	q << "BEGIN;"
			"CREATE TEMP TABLE " << tmpTable << " "
				"(LIKE slice) ON COMMIT DROP;"
			"INSERT INTO " << tmpTable <<
				"(id, section, min_x, min_y, max_x, max_y, ctr_x, "
				"ctr_y, value, size) VALUES";

	char separator = ' ';
	for (boost::shared_ptr<Slice> slice : slices) {

		std::string sliceId = boost::lexical_cast<std::string>(
				PostgreSqlUtils::hashToPostgreSqlId(slice->hashValue()));
		util::point<double, 2> ctr = slice->getComponent()->getInteriorPoint();

		// Bounding Box
		const util::box<unsigned int, 2>& bb = slice->getComponent()->getBoundingBox();

		q << separator << "(" << sliceId << "," << slice->getSection() << ",";
		q << bb.min().x() << "," << bb.min().y() << ",";
		q << bb.max().x() << "," << bb.max().y() << ",";
		q << ctr.x() << "," << ctr.y() << ",";
		q << slice->getComponent()->getValue() << ",";
		q << slice->getComponent()->getSize() << ")";

		separator = ',';
	}

	// Insert new slices from the temporary table.
	// Since slices with identical hashes are assumed to be identical, existing
	// slices are not updated.
	q << ";INSERT INTO slice "
			"(id, section, min_x, min_y, max_x, max_y, "
			"ctr_x, ctr_y, value, size) "
			"SELECT "
			"t.id, t.section, t.min_x, t.min_y, t.max_x, t.max_y, "
			"t.ctr_x, t.ctr_y, t.value, t.size "
			"FROM " << tmpTable << " AS t "
			"ON CONFLICT DO NOTHING;";
	q << "INSERT INTO slice_block_relation (block_id, slice_id) "
			"SELECT b.id, t.id "
			"FROM (" << blockQuery << ") AS b, " << tmpTable << " AS t "
			"ON CONFLICT DO NOTHING;";

	if (doneWithBlock)
		q << "UPDATE block SET slices_flag = TRUE WHERE id = (" << blockQuery << ");";

	q << "COMMIT;";
	std::string query = q.str();
	PostgreSqlUtils::waitForAsyncQuery(_pgConnection);
	int asyncStatus = PQsendQuery(_pgConnection, query.c_str());
	if (0 == asyncStatus) {
		LOG_ERROR(postgresqlslicestorelog) << "PQsendQuery returned 0" << std::endl;
		LOG_ERROR(postgresqlslicestorelog) << "The used query was: " << query <<
			std::endl;
		UTIL_THROW_EXCEPTION(PostgreSqlException, "PQsendQuery returned 0");
	}


	std::vector<std::pair<const std::string, const ConnectedComponent&> > components;
	components.reserve(slices.size());

	for (boost::shared_ptr<Slice> slice : slices) {

		std::string sliceId = boost::lexical_cast<std::string>(
				PostgreSqlUtils::hashToPostgreSqlId(slice->hashValue()));

		components.emplace_back(sliceId, *slice->getComponent());
	}

	saveConnectedComponents(components);

	boost::chrono::nanoseconds queryElapsed(queryTimer.elapsed().wall);
	LOG_DEBUG(postgresqlslicestorelog) << "Stored " << slices.size() << " slices in "
			<< (queryElapsed.count() / 1e6) << " ms (wall) ("
			<< (1e9 * slices.size()/queryElapsed.count()) << " slices/s)" << std::endl;
}

void
PostgreSqlSliceStore::associateConflictSetsToBlock(
		const ConflictSets& conflictSets, const Block& block)
{
	if (conflictSets.size() == 0)
		return;

	std::string blockQuery = PostgreSqlUtils::createBlockIdQuery(block);

	boost::timer::cpu_timer queryTimer;

	std::ostringstream idSets;
	char separator = ' ';

	// Find all conflicting slice pairs
	for (const ConflictSet& conflictSet : conflictSets)
	{
		std::string conflictSetId = boost::lexical_cast<std::string>(
				PostgreSqlUtils::hashToPostgreSqlId(hash_value(conflictSet)));
		for (SliceHash hash1 : conflictSet.getSlices())
		{
			for (SliceHash hash2 : conflictSet.getSlices())
			{
				if (hash1 < hash2)
				{
					// Create proper string representation of hashes
					std::string id1 = boost::lexical_cast<std::string>(
							PostgreSqlUtils::hashToPostgreSqlId(hash1));
					std::string id2 = boost::lexical_cast<std::string>(
							PostgreSqlUtils::hashToPostgreSqlId(hash2));

					// Insert conflicting pair
					idSets << separator << '(' << id1 << ',' << id2 << ',' << conflictSetId << ')';
					separator = ',';
				}
			}
		}
	}

	std::string idSetsStr = idSets.str();
	// If conflictSets only contained self-conflicts (which are not stored),
	// do nothing.
	if (idSetsStr.empty())
		return;

	std::ostringstream tmpTableStream;
	tmpTableStream << "slice_conflict_tmp_" << block.x() << "_" << block.y() << "_" << block.z();
	const std::string tmpTable = tmpTableStream.str();

	std::ostringstream q;
	q << "BEGIN;"
			"CREATE TEMP TABLE " << tmpTable << " "
				"(slice_a_id bigint, slice_b_id bigint, clique_id bigint) ON COMMIT DROP;"
			"INSERT INTO " << tmpTable << " VALUES" << idSetsStr << ';';
	// Insert new conflict sets from the temporary table.
	q << "INSERT INTO slice_conflict (slice_a_id, slice_b_id) "
			"SELECT DISTINCT t.slice_a_id, t.slice_b_id "
			"FROM " << tmpTable << " AS t "
			"ORDER BY t.slice_a_id ASC, t.slice_b_id ASC "
			"ON CONFLICT DO NOTHING;";
	q << "INSERT INTO conflict_clique (id, maximal_clique) "
			"SELECT DISTINCT t.clique_id, TRUE "
			"FROM " << tmpTable << " AS t "
			"ON CONFLICT DO NOTHING;";
	q << "INSERT INTO conflict_clique_edge (conflict_clique_id, slice_conflict_id) "
			"SELECT t.clique_id, sc.id "
			"FROM " << tmpTable << " AS t "
			"JOIN slice_conflict sc "
			"ON (sc.slice_a_id=t.slice_a_id AND sc.slice_b_id=t.slice_b_id) "
			"ON CONFLICT DO NOTHING;";
	q << "INSERT INTO block_conflict_relation (block_id,slice_conflict_id) "
			"SELECT DISTINCT b.id, sc.id "
			"FROM (" << blockQuery << ") AS b, " << tmpTable << " AS t "
			"JOIN slice_conflict sc "
			"ON (sc.slice_a_id=t.slice_a_id AND sc.slice_b_id=t.slice_b_id) "
			"ON CONFLICT DO NOTHING;";
	q << "COMMIT;";

	std::string query = q.str();
	PostgreSqlUtils::waitForAsyncQuery(_pgConnection);
	int asyncStatus = PQsendQuery(_pgConnection, query.c_str());
	if (0 == asyncStatus) {
		LOG_ERROR(postgresqlslicestorelog) << "PQsendQuery returned 0" << std::endl;
		LOG_ERROR(postgresqlslicestorelog) << "The used query was: " << query <<
			std::endl;
		UTIL_THROW_EXCEPTION(PostgreSqlException, "PQsendQuery returned 0");
	}

	boost::chrono::nanoseconds queryElapsed(queryTimer.elapsed().wall);
	LOG_DEBUG(postgresqlslicestorelog) << "Stored " << conflictSets.size() << " conflict sets in "
			<< (queryElapsed.count() / 1e6) << " ms (wall) ("
			<< (1e9 * conflictSets.size()/queryElapsed.count()) << " sets/s)" << std::endl;
}

boost::shared_ptr<Slices>
PostgreSqlSliceStore::getSlicesByBlocks(const Blocks& blocks, Blocks& missingBlocks)
{
	boost::shared_ptr<Slices> slices = boost::make_shared<Slices>();

	if (blocks.empty()) return slices;

	boost::timer::cpu_timer queryTimer;

	// Check if any requested block do not have slices flagged.
	PostgreSqlUtils::waitForAsyncQuery(_pgConnection);
	std::string blockIdsStr = PostgreSqlUtils::checkBlocksFlags(
			blocks, "slices_flag", missingBlocks, _pgConnection);

	if (!missingBlocks.empty()) return slices;

	// Query slices for this set of blocks
	std::string blockSlicesQuery =
			"SELECT s.id, s.section, s.value "
			"FROM slice_block_relation sbr "
			"JOIN slice s on sbr.slice_id = s.id "
			"WHERE sbr.block_id IN (" + blockIdsStr + ")"
			"GROUP BY s.id"; // Remove duplicates. GROUP BY is sometimes faster than DISTINCT.
	PGresult* result = PQexec(_pgConnection, blockSlicesQuery.c_str());

	PostgreSqlUtils::checkPostgreSqlError(result, blockSlicesQuery);
	boost::chrono::nanoseconds queryElapsed(queryTimer.elapsed().wall);

	slicesFromResult(result, slices);

	PQclear(result);

	boost::chrono::nanoseconds totalElapsed(queryTimer.elapsed().wall);
	LOG_DEBUG(postgresqlslicestorelog) << "Retrieved " << slices->size() << " slices in "
			<< (totalElapsed.count() / 1e6) << " ms (wall) "
			<< (1e9 * slices->size()/totalElapsed.count()) << " segments/s (query: "
			<< (queryElapsed.count() / 1e6) << "ms; "
			<< (1e9 * slices->size()/queryElapsed.count()) << " segments/s)" << std::endl;

	return slices;
}

boost::shared_ptr<Slices>
PostgreSqlSliceStore::getSlicesBySegmentHashes(
		const std::set<SegmentHash>& segmentHashes) {

	boost::shared_ptr<Slices> slices = boost::make_shared<Slices>();

	if (segmentHashes.size() == 0) {

		LOG_ERROR(postgresqlslicestorelog)
				<< "getSlicesBySegmentHashes() called with empty segment hash set"
				<< std::endl;

		return slices;
	}

	std::ostringstream query;
	query
			<< "BEGIN;"
			<< "CREATE TEMP TABLE segment_hash (id bigint NOT NULL) ON COMMIT DROP;"
			<< "INSERT INTO segment_hash (id) VALUES ";
	for (auto hash = segmentHashes.begin(); hash != segmentHashes.end(); hash++)
		query
				<< (hash == segmentHashes.begin() ? "" : ",")
				<< "(" << PostgreSqlUtils::hashToPostgreSqlId(*hash) << ")";
	query << ";";
	query
			<< "SELECT s.id, s.section, s.value "
			<< "FROM segment_slice ss "
			<< "JOIN slice s ON s.id = ss.slice_id "
			<< "JOIN segment_hash ON segment_hash.id = ss.segment_id "
			<< "GROUP BY s.id;";

	PGresult* result = PQexec(_pgConnection, query.str().c_str());
	PostgreSqlUtils::checkPostgreSqlError(result);

	slicesFromResult(result, slices);

	PQclear(result);

	result = PQexec(_pgConnection, "COMMIT");
	PostgreSqlUtils::checkPostgreSqlError(result, "COMMIT");
	PQclear(result);

	return slices;
}

boost::shared_ptr<ConflictSets>
PostgreSqlSliceStore::getConflictSetsByBlocks(
			const Blocks& blocks,
			Blocks&       missingBlocks) {

	boost::shared_ptr<ConflictSets> conflictSets = boost::make_shared<ConflictSets>();

	if (blocks.empty()) return conflictSets;

	boost::timer::cpu_timer queryTimer;

	// Check if any requested block do not have slices flagged.
	PostgreSqlUtils::waitForAsyncQuery(_pgConnection);
	std::string blockIdsStr = PostgreSqlUtils::checkBlocksFlags(
			blocks,	"slices_flag", missingBlocks, _pgConnection);

	if (!missingBlocks.empty()) return conflictSets;

	// Query conflict sets for this set of blocks
	// Note that this only retrieves conflict edges referenced by conflict
	// cliques. While currently this includes all relevant conflict edges, that
	// may change in the future.
	std::string blockConflictsQuery =
			"SELECT cc.id, cc.maximal_clique, "
			"ARRAY_AGG(DISTINCT sc.slice_a_id) || ARRAY_AGG(DISTINCT sc.slice_b_id) "
			"FROM conflict_clique cc "
			"LEFT OUTER JOIN conflict_clique_edge cce "
			"ON cce.conflict_clique_id = cc.id "
			"JOIN slice_conflict sc "
			"ON sc.id = cce.slice_conflict_id "
			"JOIN block_conflict_relation bcr ON bcr.slice_conflict_id = sc.id "
			"WHERE bcr.block_id IN (" + blockIdsStr + ") "
			"GROUP BY cc.id, cc.maximal_clique;";
	enum { FIELD_CLIQUE_ID, FIELD_MAXIMAL_CLIQUE, FIELD_SLICE_IDS };
	PGresult* result = PQexec(_pgConnection, blockConflictsQuery.c_str());

	PostgreSqlUtils::checkPostgreSqlError(result, blockConflictsQuery);
	int nConflicts = PQntuples(result);

	// Build ConflictSet for each row
	for (int i = 0; i < nConflicts; ++i) {
		ConflictSet conflictSet;

		char* cellStr;
		cellStr = PQgetvalue(result, i, FIELD_SLICE_IDS);
		std::string sliceIds(cellStr);
		boost::char_separator<char> separator("{}()\", \t");
		boost::tokenizer<boost::char_separator<char> > sliceTokens(sliceIds, separator);

		for (const std::string& sliceId : sliceTokens) {
			SliceHash sliceHash = PostgreSqlUtils::postgreSqlIdToHash(
					boost::lexical_cast<PostgreSqlHash>(sliceId));
			conflictSet.addSlice(sliceHash);
		}

		cellStr = PQgetvalue(result, i, FIELD_MAXIMAL_CLIQUE);
		conflictSet.setMaximalClique(cellStr[0] == 't');

		conflictSets->add(conflictSet);
	}

	PQclear(result);

	boost::chrono::nanoseconds queryElapsed(queryTimer.elapsed().wall);
	LOG_DEBUG(postgresqlslicestorelog) << "Retrieved " << conflictSets->size() << " conflict sets in "
			<< (queryElapsed.count() / 1e6) << " ms (wall) ("
			<< (1e9 * conflictSets->size()/queryElapsed.count()) << " sets/s)" << std::endl;

	return conflictSets;
}

bool
PostgreSqlSliceStore::getSlicesFlag(const Block& block) {

	const std::string blockQuery = PostgreSqlUtils::createBlockIdQuery(block);
	std::string blockFlagQuery = "SELECT slices_flag FROM block "
			"WHERE id = (" + blockQuery + ")";
	PostgreSqlUtils::waitForAsyncQuery(_pgConnection);
	PGresult* queryResult = PQexec(_pgConnection, blockFlagQuery.c_str());

	PostgreSqlUtils::checkPostgreSqlError(queryResult, blockFlagQuery);

	bool result = 0 == strcmp(PQgetvalue(queryResult, 0, 0), "t");

	PQclear(queryResult);

	return result;
}

void
PostgreSqlSliceStore::saveConnectedComponent(const std::string& slicePostgreId, const ConnectedComponent& component)
{
	std::string imageFilename  = "/dev/shm/catsop" + slicePostgreId + ".png";

	// If the image file already exists, do nothing.
	struct stat buffer;
	if (stat (imageFilename.c_str(), &buffer) == 0) return;

	const ConnectedComponent::bitmap_type& bitmap = component.getBitmap();
	const vigra::Diff2D offset(component.getBoundingBox().min().x(), component.getBoundingBox().min().y());

	// store the image
	vigra::exportImage(
			vigra::srcImageRange(bitmap),
			vigra::ImageExportInfo(imageFilename.c_str()).setPosition(offset));

	std::ostringstream q;
	q
			<< "INSERT INTO slice_component (slice_id, component) VALUES ("
			<< slicePostgreId << ", E'\\\\x";

	unsigned char x;
	std::ifstream input(imageFilename, std::ios::binary);
	input >> std::noskipws;
	while (input >> x) {
		q << std::hex << std::setw(2) << std::setfill('0')
				<< (int)x;
	}
	q << "') ON CONFLICT (slice_id) DO NOTHING;";

	std::string query = q.str();
	PostgreSqlUtils::waitForAsyncQuery(_pgConnection);
	int asyncStatus = PQsendQuery(_pgConnection, query.c_str());
	if (0 == asyncStatus) {
		LOG_ERROR(postgresqlslicestorelog) << "PQsendQuery returned 0" << std::endl;
		LOG_ERROR(postgresqlslicestorelog) << "The used query was: " << query <<
			std::endl;
		UTIL_THROW_EXCEPTION(PostgreSqlException, "PQsendQuery returned 0");
	}

	input.close();
	bool removeFailed = 0 != std::remove(imageFilename.c_str());
	if (removeFailed) {
		LOG_ERROR(postgresqlslicestorelog) << "Failed to delete tmp component file: " << imageFilename << std::endl;
		UTIL_THROW_EXCEPTION(PostgreSqlException, "Failed to delete tmp component file");
	}
}

void
PostgreSqlSliceStore::saveConnectedComponents(const std::vector<std::pair<const std::string, const ConnectedComponent&> >& components) {
	std::ostringstream q;
	q << "INSERT INTO slice_component (slice_id, component) VALUES";
	char separator = ' ';
	unsigned int actualCount = 0;

	for (const std::pair<const std::string, const ConnectedComponent&>& compPair : components) {
		const std::string& slicePostgreId = compPair.first;
		const ConnectedComponent& component = compPair.second;

		std::string imageFilename  = "/dev/shm/catsop" + slicePostgreId + ".png";

		// If the image file already exists, do nothing.
		struct stat buffer;
		if (stat (imageFilename.c_str(), &buffer) == 0) continue;
		actualCount++;

		const ConnectedComponent::bitmap_type& bitmap = component.getBitmap();
		const vigra::Diff2D offset(component.getBoundingBox().min().x(), component.getBoundingBox().min().y());

		// store the image
		vigra::exportImage(
				vigra::srcImageRange(bitmap),
				vigra::ImageExportInfo(imageFilename.c_str()).setPosition(offset));

		q << separator << '(' << slicePostgreId << ", E'\\\\x";
		separator = ',';

		unsigned char x;
		std::ifstream input(imageFilename, std::ios::binary);
		input >> std::noskipws;
		while (input >> x) {
			q << std::hex << std::setw(2) << std::setfill('0')
					<< (int)x;
		}
		q << "')";


		input.close();
		bool removeFailed = 0 != std::remove(imageFilename.c_str());
		if (removeFailed) {
			LOG_ERROR(postgresqlslicestorelog) << "Failed to delete tmp component file: " << imageFilename << std::endl;
			UTIL_THROW_EXCEPTION(PostgreSqlException, "Failed to delete tmp component file");
		}
	}

	if (0 == actualCount) return;

	q << " ON CONFLICT (slice_id) DO NOTHING;";

	std::string query = q.str();
	PostgreSqlUtils::waitForAsyncQuery(_pgConnection);
	int asyncStatus = PQsendQuery(_pgConnection, query.c_str());
	if (0 == asyncStatus) {
		LOG_ERROR(postgresqlslicestorelog) << "PQsendQuery returned 0" << std::endl;
		LOG_ERROR(postgresqlslicestorelog) << "The used query was: " << query <<
			std::endl;
		UTIL_THROW_EXCEPTION(PostgreSqlException, "PQsendQuery returned 0");
	}
}

boost::shared_ptr<ConnectedComponent>
PostgreSqlSliceStore::loadConnectedComponent(const std::string& slicePostgreId, double value)
{
	std::string imageFilename  = _config.getComponentDirectory() + "/" + slicePostgreId + ".png";

	// get information about the image to read
	vigra::ImageImportInfo info(imageFilename.c_str());

	// abort if image is not grayscale
	if (!info.isGrayscale()) {

		UTIL_THROW_EXCEPTION(
				IOError,
				imageFilename << " is not a gray-scale image!");
	}

	// read the image
	ConnectedComponent::bitmap_type bitmap(ConnectedComponent::bitmap_type::difference_type(info.width(), info.height()));
	importImage(info, vigra::destImage(bitmap));

	const vigra::Diff2D offset = info.getPosition();

	// Vigra png normalization workaround: rectangular slices are stored as
	// black only. Hence, if the sum of all pixels is zero, all pixels of the
	// bounding box belong to the slice.
	size_t nNonzero = bitmap.sum<size_t>();

	if (!nNonzero) {

		bitmap = true;
		nNonzero = info.width() * info.height();
	}

	// create the component
	boost::shared_ptr<ConnectedComponent> component = boost::make_shared<ConnectedComponent>(
			boost::shared_ptr<Image>(),
			value,
			util::point<int, 2>(offset.x, offset.y),
			bitmap,
			nNonzero);

	return component;
}

void
PostgreSqlSliceStore::slicesFromResult(PGresult* result, boost::shared_ptr<Slices> slices) {

	int nSlices = PQntuples(result);

	enum { FIELD_ID, FIELD_SECTION, FIELD_VALUE };

	// Build Slice for each row
	for (int i = 0; i < nSlices; ++i) {
		char* cellStr;
		cellStr = PQgetvalue(result, i, FIELD_ID);
		std::string slicePostgreId(cellStr);
		SliceHash sliceHash = PostgreSqlUtils::postgreSqlIdToHash(
				boost::lexical_cast<PostgreSqlHash>(slicePostgreId));
		cellStr = PQgetvalue(result, i, FIELD_SECTION);
		unsigned int section = boost::lexical_cast<unsigned int>(cellStr);
		cellStr = PQgetvalue(result, i, FIELD_VALUE);
		double value = boost::lexical_cast<double>(cellStr);

		boost::shared_ptr<Slice> slice = boost::make_shared<Slice>(
				ComponentTreeConverter::getNextSliceId(),
				section,
				loadConnectedComponent(slicePostgreId, value));

		// Check that the loaded slice has the correct hash.
		if (slice->hashValue() != sliceHash) {
			std::ostringstream errorMsg;
			errorMsg << "Retrieved slice has wrong hash. Original: " << sliceHash <<
					" Retrieved: " << slice->hashValue();

			LOG_ERROR(postgresqlslicestorelog) << errorMsg.str() << std::endl;
			UTIL_THROW_EXCEPTION(PostgreSqlException, errorMsg.str());
		}

		slices->add(slice);
	}
}

#endif // HAVE_PostgreSQL
