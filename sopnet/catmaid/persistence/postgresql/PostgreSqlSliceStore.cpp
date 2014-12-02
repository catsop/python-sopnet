#include "config.h"
#ifdef HAVE_PostgreSQL

#include "PostgreSqlUtils.h"
#include <boost/timer/timer.hpp>
#include <fstream>
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
}

PostgreSqlSliceStore::~PostgreSqlSliceStore() {

	if (_pgConnection != 0) {
		PostgreSqlUtils::waitForAsyncQuery(_pgConnection);
		PQfinish(_pgConnection);
	}
}

void
PostgreSqlSliceStore::associateSlicesToBlock(const Slices& slices, const Block& block, bool doneWithBlock) {

	if (slices.size() == 0)
		return;

	boost::timer::cpu_timer queryTimer;

	unsigned int stack_id = _config.getCatmaidRawStackId();
	std::string blockQuery = PostgreSqlUtils::createBlockIdQuery(
				block, stack_id);

	std::ostringstream tmpTableStream;
	tmpTableStream << "djsopnet_slice_tmp_" << block.x() << "_" << block.y() << "_" << block.z();
	const std::string tmpTable = tmpTableStream.str();

	std::ostringstream q;
	q << "BEGIN;"
			"CREATE TEMP TABLE " << tmpTable << " "
				"(LIKE djsopnet_slice) ON COMMIT DROP;"
			"INSERT INTO " << tmpTable <<
				"(stack_id, section, min_x, min_y, max_x, max_y, ctr_x, "
				"ctr_y, value, size, id) VALUES";

	char separator = ' ';
	foreach (boost::shared_ptr<Slice> slice, slices)
	{
		std::string hash = boost::lexical_cast<std::string>(
				PostgreSqlUtils::hashToPostgreSqlId(slice->hashValue()));
		util::point<double> ctr = slice->getComponent()->getCenter();

		// Store pixel data of slice
		saveConnectedComponent(hash, *slice->getComponent());

		// Bounding Box
		const util::rect<unsigned int>& bb = slice->getComponent()->getBoundingBox();

		q << separator << "(" << stack_id << "," << slice->getSection() << ",";
		q << bb.minX << "," << bb.minY << ",";
		q << bb.maxX << "," << bb.maxY << ",";
		q << ctr.x << "," << ctr.y << ",";
		q << slice->getComponent()->getValue() << ",";
		q << slice->getComponent()->getSize() << ",";
		q << hash << ")";

		separator = ',';
	}

	// Insert new slices from the temporary table.
	q << ";LOCK TABLE djsopnet_slice IN EXCLUSIVE MODE;";
	// Since slices with identical hashes are assumed to be identical, existing
	// slices are not updated. If needed, that update would be:
	/* "UPDATE djsopnet_slice "
		"SET (stack_id, section, min_x, min_y, max_x, max_y, "
		"ctr_x, ctr_y, value, size)="
		"(t.stack_id, t.section, t.min_x, t.min_y, t.max_x, t.max_y, "
		"t.ctr_x, t.ctr_y, t.value, t.size) "
		"FROM " << tmpTable << " AS t "
		"WHERE t.id = djsopnet_slice.id;" */
	q << "INSERT INTO djsopnet_slice "
			"(stack_id, section, min_x, min_y, max_x, max_y, "
			"ctr_x, ctr_y, value, size, id) "
			"SELECT "
			"t.stack_id, t.section, t.min_x, t.min_y, t.max_x, t.max_y, "
			"t.ctr_x, t.ctr_y, t.value, t.size, t.id "
			"FROM " << tmpTable << " AS t "
			"LEFT OUTER JOIN djsopnet_slice s "
			"ON (s.id = t.id) WHERE s.id IS NULL;";
	q << "INSERT INTO djsopnet_sliceblockrelation (block_id, slice_id) "
			"SELECT b.id, t.id "
			"FROM (" << blockQuery << ") AS b, " << tmpTable << " AS t "
			"WHERE NOT EXISTS "
			"(SELECT 1 FROM djsopnet_sliceblockrelation s WHERE "
			"(s.block_id, s.slice_id) = (b.id, t.id));";

	if (doneWithBlock)
		q << "UPDATE djsopnet_block SET slices_flag = TRUE WHERE id = (" << blockQuery << ");";

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

	std::string blockQuery = PostgreSqlUtils::createBlockIdQuery(
			block, _config.getCatmaidRawStackId());

	boost::timer::cpu_timer queryTimer;

	std::ostringstream idSets;
	idSets << "(VALUES";
	char separator = ' ';

	// Find all conflicting slice pairs
	foreach (const ConflictSet& conflictSet, conflictSets)
	{
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
					idSets << separator << '(' << id1 << ',' << id2 << ')';
					separator = ',';
				}
			}
		}
	}

	idSets << ')';
	std::string idSetsStr = idSets.str();

	std::ostringstream tmpTableStream;
	tmpTableStream << "djsopnet_sliceconflictset_tmp_" << block.x() << "_" << block.y() << "_" << block.z();
	const std::string tmpTable = tmpTableStream.str();

	std::ostringstream q;
	q << "BEGIN;"
			"CREATE TEMP TABLE " << tmpTable << " "
				"(LIKE djsopnet_sliceconflictset) ON COMMIT DROP;"
			"ALTER TABLE " << tmpTable << " ALTER id DROP NOT NULL;"
			"INSERT INTO " << tmpTable << " (slice_a_id,slice_b_id) "
				"(SELECT DISTINCT * FROM " << idSetsStr << " AS h (a,b));";
	// Insert new conflict sets from the temporary table.
	q << "LOCK TABLE djsopnet_sliceconflictset IN EXCLUSIVE MODE;";
	q << "INSERT INTO djsopnet_sliceconflictset (slice_a_id,slice_b_id) "
			"SELECT t.slice_a_id,t.slice_b_id "
			"FROM " << tmpTable << " AS t "
			"LEFT OUTER JOIN djsopnet_sliceconflictset s "
			"ON (s.slice_a_id,s.slice_b_id) = (t.slice_a_id,t.slice_b_id) "
			"WHERE s.slice_a_id IS NULL;";
	q << "INSERT INTO djsopnet_blockconflictrelation (block_id,conflict_id) "
			"SELECT b.id, c.id "
			"FROM (" << blockQuery << ") AS b, " << tmpTable << " AS t "
			"JOIN djsopnet_sliceconflictset c "
			"ON (c.slice_a_id=t.slice_a_id AND c.slice_b_id=t.slice_b_id) "
			"WHERE NOT EXISTS "
			"(SELECT 1 FROM djsopnet_blockconflictrelation bc WHERE "
			"(bc.block_id, bc.conflict_id) = (b.id, c.id));";
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
			blocks, _config.getCatmaidRawStackId(),
			"slices_flag", missingBlocks, _pgConnection);

	if (!missingBlocks.empty()) return slices;

	// Query slices for this set of blocks
	std::string blockSlicesQuery =
			"SELECT s.id, s.section "
			"FROM djsopnet_sliceblockrelation sbr "
			"JOIN djsopnet_slice s on sbr.slice_id = s.id "
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
			blocks, _config.getCatmaidRawStackId(),
			"slices_flag", missingBlocks, _pgConnection);

	if (!missingBlocks.empty()) return conflictSets;

	// Query conflict sets for this set of blocks
	std::string blockConflictsQuery =
			"SELECT DISTINCT cs.slice_a_id, cs.slice_b_id "
			"FROM djsopnet_sliceconflictset cs "
			"JOIN djsopnet_blockconflictrelation bcr ON bcr.conflict_id = cs.id "
			"WHERE bcr.block_id IN (" + blockIdsStr + ")";
	enum { FIELD_SLICE_A_ID, FIELD_SLICE_B_ID };
	PGresult* result = PQexec(_pgConnection, blockConflictsQuery.c_str());

	PostgreSqlUtils::checkPostgreSqlError(result, blockConflictsQuery);
	int nConflicts = PQntuples(result);

	// Build SegmentDescription for each row
	for (int i = 0; i < nConflicts; ++i) {
		char* cellStr;
		cellStr = PQgetvalue(result, i, FIELD_SLICE_A_ID);
		SliceHash sliceAHash = PostgreSqlUtils::postgreSqlIdToHash(
				boost::lexical_cast<PostgreSqlHash>(cellStr));
		cellStr = PQgetvalue(result, i, FIELD_SLICE_B_ID);
		SliceHash sliceBHash = PostgreSqlUtils::postgreSqlIdToHash(
				boost::lexical_cast<PostgreSqlHash>(cellStr));

		ConflictSet conflictSet;
		conflictSet.addSlice(sliceAHash);
		conflictSet.addSlice(sliceBHash);

		// Unnecessary because ConflictSets are not maximal cliques by default,
		// but it is still best to be explicit:
		conflictSet.setMaximalClique(false);

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

	const std::string blockQuery = PostgreSqlUtils::createBlockIdQuery(
				block, _config.getCatmaidRawStackId());
	std::string blockFlagQuery = "SELECT slices_flag FROM djsopnet_block "
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
