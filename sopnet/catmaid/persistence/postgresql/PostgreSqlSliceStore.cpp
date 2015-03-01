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
#include <sopnet/slices/ComponentTreeConverter.h>
#include <util/ProgramOptions.h>
#include <util/httpclient.h>
#include <util/box.hpp>
#include <util/point.hpp>
#include "PostgreSqlSliceStore.h"
#include <sopnet/slices/SliceHash.h>

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
	  << _config.getCatmaidStackIds(Membrane).segmentation_id
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
	foreach (boost::shared_ptr<Slice> slice, slices)
	{
		std::string sliceId = boost::lexical_cast<std::string>(
				PostgreSqlUtils::hashToPostgreSqlId(slice->hashValue()));
		util::point<double> ctr = slice->getComponent()->getCenter();

		// Store pixel data of slice
		saveConnectedComponent(sliceId, *slice->getComponent());

		// Bounding Box
		const util::rect<unsigned int>& bb = slice->getComponent()->getBoundingBox();

		q << separator << "(" << sliceId << "," << slice->getSection() << ",";
		q << bb.minX << "," << bb.minY << ",";
		q << bb.maxX << "," << bb.maxY << ",";
		q << ctr.x << "," << ctr.y << ",";
		q << slice->getComponent()->getValue() << ",";
		q << slice->getComponent()->getSize() << ")";

		separator = ',';
	}

	// Insert new slices from the temporary table.
	q << ";LOCK TABLE slice IN EXCLUSIVE MODE;";
	// Since slices with identical hashes are assumed to be identical, existing
	// slices are not updated. If needed, that update would be:
	/* "UPDATE slice "
		"SET (section, min_x, min_y, max_x, max_y, "
		"ctr_x, ctr_y, value, size)="
		"(t.section, t.min_x, t.min_y, t.max_x, t.max_y, "
		"t.ctr_x, t.ctr_y, t.value, t.size) "
		"FROM " << tmpTable << " AS t "
		"WHERE t.id = slice.id;" */
	q << "INSERT INTO slice "
			"(id, section, min_x, min_y, max_x, max_y, "
			"ctr_x, ctr_y, value, size) "
			"SELECT "
			"t.id, t.section, t.min_x, t.min_y, t.max_x, t.max_y, "
			"t.ctr_x, t.ctr_y, t.value, t.size "
			"FROM " << tmpTable << " AS t "
			"LEFT OUTER JOIN slice s "
			"ON (s.id = t.id) WHERE s.id IS NULL;";
	q << "INSERT INTO slice_block_relation (block_id, slice_id) "
			"SELECT b.id, t.id "
			"FROM (" << blockQuery << ") AS b, " << tmpTable << " AS t "
			"WHERE NOT EXISTS "
			"(SELECT 1 FROM slice_block_relation s WHERE "
			"(s.block_id, s.slice_id) = (b.id, t.id));";

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
	foreach (const ConflictSet& conflictSet, conflictSets)
	{
		std::string conflictSetId = boost::lexical_cast<std::string>(
				PostgreSqlUtils::hashToPostgreSqlId(hash_value(conflictSet)));
		foreach (SliceHash hash1, conflictSet.getSlices())
		{
			foreach (SliceHash hash2, conflictSet.getSlices())
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
	q << "LOCK TABLE slice_conflict IN EXCLUSIVE MODE;";
	q << "INSERT INTO slice_conflict (slice_a_id, slice_b_id) "
			"SELECT DISTINCT t.slice_a_id, t.slice_b_id "
			"FROM " << tmpTable << " AS t "
			"LEFT OUTER JOIN slice_conflict s "
			"ON (s.slice_a_id, s.slice_b_id) = (t.slice_a_id, t.slice_b_id) "
			"WHERE s.slice_a_id IS NULL;";
	q << "INSERT INTO conflict_clique (id, maximal_clique) "
			"SELECT DISTINCT t.clique_id, TRUE "
			"FROM " << tmpTable << " AS t "
			"LEFT OUTER JOIN conflict_clique c "
			"ON c.id = t.clique_id "
			"WHERE c.id IS NULL;";
	q << "INSERT INTO conflict_clique_edge (conflict_clique_id, slice_conflict_id) "
			"SELECT t.clique_id, sc.id "
			"FROM " << tmpTable << " AS t "
			"JOIN slice_conflict sc "
			"ON (sc.slice_a_id=t.slice_a_id AND sc.slice_b_id=t.slice_b_id) "
			"WHERE NOT EXISTS "
			"(SELECT 1 FROM conflict_clique_edge cce WHERE "
			"(cce.conflict_clique_id,cce.slice_conflict_id) = (t.clique_id, sc.id));";
	q << "INSERT INTO block_conflict_relation (block_id,slice_conflict_id) "
			"SELECT DISTINCT b.id, sc.id "
			"FROM (" << blockQuery << ") AS b, " << tmpTable << " AS t "
			"JOIN slice_conflict sc "
			"ON (sc.slice_a_id=t.slice_a_id AND sc.slice_b_id=t.slice_b_id) "
			"WHERE NOT EXISTS "
			"(SELECT 1 FROM block_conflict_relation bc WHERE "
			"(bc.block_id, bc.slice_conflict_id) = (b.id, sc.id));";
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
			"SELECT s.id, s.section "
			"FROM slice_block_relation sbr "
			"JOIN slice s on sbr.slice_id = s.id "
			"WHERE sbr.block_id IN (" + blockIdsStr + ")";
	enum { FIELD_ID, FIELD_SECTION };
	PGresult* result = PQexec(_pgConnection, blockSlicesQuery.c_str());

	PostgreSqlUtils::checkPostgreSqlError(result, blockSlicesQuery);
	int nSlices = PQntuples(result);

	// Build SegmentDescription for each row
	for (int i = 0; i < nSlices; ++i) {
		char* cellStr;
		cellStr = PQgetvalue(result, i, FIELD_ID);
		std::string slicePostgreId(cellStr);
		SliceHash sliceHash = PostgreSqlUtils::postgreSqlIdToHash(
				boost::lexical_cast<PostgreSqlHash>(slicePostgreId));
		cellStr = PQgetvalue(result, i, FIELD_SECTION);
		unsigned int section = boost::lexical_cast<unsigned int>(cellStr);

		boost::shared_ptr<Slice> slice = boost::make_shared<Slice>(
				ComponentTreeConverter::getNextSliceId(),
				section,
				loadConnectedComponent(slicePostgreId));

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

	PQclear(result);

	boost::chrono::nanoseconds queryElapsed(queryTimer.elapsed().wall);
	LOG_DEBUG(postgresqlslicestorelog) << "Retrieved " << slices->size() << " slices in "
			<< (queryElapsed.count() / 1e6) << " ms (wall) ("
			<< (1e9 * slices->size()/queryElapsed.count()) << " slices/s)" << std::endl;

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

		foreach (const std::string& sliceId, sliceTokens) {
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
PostgreSqlSliceStore::saveConnectedComponent(std::string slicePostgreId, const ConnectedComponent& component)
{
	std::string imageFilename  = _config.getComponentDirectory() + "/" + slicePostgreId + ".png";
	std::string offsetFilename = _config.getComponentDirectory() + "/" + slicePostgreId + ".off";

	// If the image file already exists, do nothing.
	struct stat buffer;
	if (stat (imageFilename.c_str(), &buffer) == 0) return;

	const ConnectedComponent::bitmap_type& bitmap = component.getBitmap();

	// store the image
	vigra::exportImage(
			vigra::srcImageRange(bitmap),
			vigra::ImageExportInfo(imageFilename.c_str()));

	std::ofstream componentFile(offsetFilename.c_str());

	// store the component's value
	componentFile << component.getValue() << " ";

	// store the component's offset
	componentFile << component.getBoundingBox().minX << " ";
	componentFile << component.getBoundingBox().minY;
}

boost::shared_ptr<ConnectedComponent>
PostgreSqlSliceStore::loadConnectedComponent(std::string slicePostgreId)
{
	std::string imageFilename  = _config.getComponentDirectory() + "/" + slicePostgreId + ".png";
	std::string offsetFilename = _config.getComponentDirectory() + "/" + slicePostgreId + ".off";

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

	// open the offset file
	std::ifstream componentFile(offsetFilename.c_str());

	if (!componentFile) {

		UTIL_THROW_EXCEPTION(
				IOError,
				offsetFilename << " failed to open!");
	}

	// load the component's value
	double value;
	componentFile >> value;

	// load the component's offset
	unsigned int offsetX, offsetY;
	componentFile >> offsetX;
	componentFile >> offsetY;

	// create a pixel list
	boost::shared_ptr<ConnectedComponent::pixel_list_type> pixelList =
			boost::make_shared<ConnectedComponent::pixel_list_type>();

	// fill it with white pixels from the bitmap
	for (unsigned int x = 0; x < static_cast<unsigned int>(info.width()); x++)
		for (unsigned int y = 0; y < static_cast<unsigned int>(info.height()); y++)
			if (bitmap(x, y) == 1.0)
				pixelList->push_back(util::point<unsigned int>(offsetX + x, offsetY + y));

	// create the component
	boost::shared_ptr<ConnectedComponent> component = boost::make_shared<ConnectedComponent>(
			boost::shared_ptr<Image>(),
			value,
			pixelList,
			0,
			pixelList->size());

	return component;
}

#endif // HAVE_PostgreSQL
