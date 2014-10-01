#include "config.h"
#ifdef HAVE_PostgreSQL

#include "PostgreSqlUtils.h"
#include <fstream>
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
        const ProjectConfiguration& config) : _config(config), _blockUtils(config)
{
	_pgConnection = PostgreSqlUtils::getConnection(
			_config.getPostgreSqlHost(),
			_config.getPostgreSqlDatabase(),
			_config.getPostgreSqlUser(),
			_config.getPostgreSqlPassword());
}

PostgreSqlSliceStore::~PostgreSqlSliceStore() {

	if (_pgConnection != 0)
		PQfinish(_pgConnection);
}

void
PostgreSqlSliceStore::associateSlicesToBlock(const Slices& slices, const Block& block) {

	if (slices.size() == 0)
		return;

	PGresult *result;
	unsigned int stack_id = _config.getCatmaidRawStackId();
	std::string blockQuery = PostgreSqlUtils::createBlockIdQuery(
				_blockUtils, block, stack_id);

	foreach (boost::shared_ptr<Slice> slice, slices)
	{
		std::string hash = boost::lexical_cast<std::string>(
				PostgreSqlUtils::hashToPostgreSqlId(slice->hashValue()));
		util::point<double> ctr = slice->getComponent()->getCenter();

		// Store pixel data of slice
		saveConnectedComponent(hash, *slice->getComponent());

		// Bounding Box
		const util::rect<unsigned int>& bb = slice->getComponent()->getBoundingBox();

		// Create slices in slice table
		// TODO: Use upsert statement, based on CTEs
		std::ostringstream q;
		q << "INSERT INTO djsopnet_slice ";
		q << "(stack_id, section, min_x, min_y, max_x, max_y, ctr_x, "
		<< "ctr_y, value, size, id) VALUES (";
		q << stack_id << "," << slice->getSection() << ",";
		q << bb.minX << "," << bb.minY << ",";
		q << bb.maxX << "," << bb.maxY << ",";
		q << ctr.x << "," << ctr.y << ",";
		q << slice->getComponent()->getValue() << ",";
		q << slice->getComponent()->getSize() << ",";
		q << hash << ")";

		std::string query = q.str();
		result = PQexec(_pgConnection, query.c_str());
		PostgreSqlUtils::checkPostgreSqlError(result, query);
		PQclear(result);

		std::ostringstream q2;
		q2 << "INSERT INTO djsopnet_sliceblockrelation (block_id, slice_id) ";
		q2 << "VALUES ((" << blockQuery << ")," << hash << ")";

		std::string query2 = q2.str();
		result = PQexec(_pgConnection, query2.c_str());
		PostgreSqlUtils::checkPostgreSqlError(result, query2);
		PQclear(result);
	}

	std::string blockFlagQuery =
			"UPDATE djsopnet_block SET slices_flag = TRUE WHERE id = (" + blockQuery + ")";
	result = PQexec(_pgConnection, blockFlagQuery.c_str());

	PostgreSqlUtils::checkPostgreSqlError(result, blockFlagQuery);
	PQclear(result);
}

void
PostgreSqlSliceStore::associateConflictSetsToBlock(
		const ConflictSets& conflictSets, const Block& block)
{
	if (conflictSets.size() == 0)
		return;

	std::string blockQuery = PostgreSqlUtils::createBlockIdQuery(
			_blockUtils, block, _config.getCatmaidRawStackId());

	// Find all conflicting slice pairs
	foreach (const ConflictSet& conflictSet, conflictSets)
	{
		foreach (SliceHash id1, conflictSet.getSlices())
		{
			foreach (SliceHash id2, conflictSet.getSlices())
			{
				if (id1 < id2)
				{
					// Create proper string representation of hashes
					std::string hash1 = boost::lexical_cast<std::string>(
							PostgreSqlUtils::hashToPostgreSqlId(id1));
					std::string hash2 = boost::lexical_cast<std::string>(
							PostgreSqlUtils::hashToPostgreSqlId(id2));

					// Insert all conflicting pairs
					std::ostringstream q;
					q << "WITH rows AS (";
					q << "INSERT INTO djsopnet_sliceconflictset ";
					q << "(slice_a_id, slice_b_id) VALUES ";
					q << "(" << hash1 << "," << hash2 << ")";
					q << "RETURNING id)";

					// Associate conflict set to block
					q << "INSERT INTO djsopnet_blockconflictrelation ";
					q << "(block_id, conflict_id) VALUES ";
					q << "((" << blockQuery << "),";
					q << "(SELECT id FROM rows))";

					std::string query = q.str();
					PGresult *result = PQexec(_pgConnection, query.c_str());
					PostgreSqlUtils::checkPostgreSqlError(result, query);
					PQclear(result);
				}
			}
		}
	}
}

boost::shared_ptr<Slices>
PostgreSqlSliceStore::getSlicesByBlocks(const Blocks& blocks, Blocks& missingBlocks)
{
	boost::shared_ptr<Slices> slices = boost::make_shared<Slices>();

	if (blocks.empty()) {
		return slices;
	}

	std::ostringstream blockIds;
	PGresult* result;

	// Check if any requested block do not have slices flagged.
	foreach (const Block& block, blocks) {
		const std::string blockQuery = PostgreSqlUtils::createBlockIdQuery(
				_blockUtils, block, _config.getCatmaidRawStackId());
		std::string blockFlagQuery = "SELECT id, slices_flag FROM djsopnet_block "
				"WHERE id = (" + blockQuery + ")";
		result = PQexec(_pgConnection, blockFlagQuery.c_str());

		PostgreSqlUtils::checkPostgreSqlError(result, blockFlagQuery);

		if (0 != strcmp(PQgetvalue(result, 0, 1), "t")) {
			missingBlocks.add(block);
		}

		blockIds << PQgetvalue(result, 0, 0) << ",";

		PQclear(result);
	}

	if (!missingBlocks.empty()) return slices;

	std::string blockIdsStr = blockIds.str();
	blockIdsStr.erase(blockIdsStr.length() - 1); // Remove trailing comma.

	// Query slices for this set of blocks
	std::string blockSlicesQuery =
			"SELECT s.id, s.section "
			"FROM djsopnet_sliceblockrelation sbr "
			"JOIN djsopnet_slice s on sbr.slice_id = s.id "
			"WHERE sbr.block_id IN (" + blockIdsStr + ")";
	enum { FIELD_ID, FIELD_SECTION };
	result = PQexec(_pgConnection, blockSlicesQuery.c_str());

	PostgreSqlUtils::checkPostgreSqlError(result, blockSlicesQuery);
	int nSlices = PQntuples(result);

	// Build SegmentDescription for each row
	for (int i = 0; i < nSlices; ++i) {
		char* cellStr;
		cellStr = PQgetvalue(result, i, FIELD_ID);
		std::string slicePostgreId(cellStr);
		cellStr = PQgetvalue(result, i, FIELD_SECTION);
		unsigned int section = boost::lexical_cast<unsigned int>(cellStr);

		boost::shared_ptr<Slice> slice = boost::make_shared<Slice>(
				0 /* TODO: how to get slice id? */,
				section,
				loadConnectedComponent(slicePostgreId));

		slices->add(slice);
	}

	PQclear(result);

	return slices;
}

boost::shared_ptr<ConflictSets>
PostgreSqlSliceStore::getConflictSetsByBlocks(
			const Blocks& blocks,
			Blocks&       missingBlocks) {

	boost::shared_ptr<ConflictSets> conflictSets = boost::make_shared<ConflictSets>();

	if (blocks.empty()) {
		return conflictSets;
	}

	std::ostringstream blockIds;
	PGresult* result;

	// Check if any requested block do not have slices flagged.
	// TODO: should conflict sets have own flag?
	foreach (const Block& block, blocks) {
		const std::string blockQuery = PostgreSqlUtils::createBlockIdQuery(
				_blockUtils, block, _config.getCatmaidRawStackId());
		std::string blockFlagQuery = "SELECT id, slices_flag FROM djsopnet_block "
				"WHERE id = (" + blockQuery + ")";
		result = PQexec(_pgConnection, blockFlagQuery.c_str());

		PostgreSqlUtils::checkPostgreSqlError(result, blockFlagQuery);

		if (0 != strcmp(PQgetvalue(result, 0, 1), "t")) {
			missingBlocks.add(block);
		}

		blockIds << PQgetvalue(result, 0, 0) << ",";

		PQclear(result);
	}

	if (!missingBlocks.empty()) return conflictSets;

	std::string blockIdsStr = blockIds.str();
	blockIdsStr.erase(blockIdsStr.length() - 1); // Remove trailing comma.

	// Query conflict sets for this set of blocks
	std::string blockConflictsQuery =
			"SELECT cs.slice_a_id, cs.slice_b_id "
			"FROM djsopnet_blockconflictrelation bcr "
			"JOIN djsopnet_sliceconflictset cs on bcr.conflict_id = cs.id "
			"WHERE bcr.block_id IN (" + blockIdsStr + ")";
	enum { FIELD_SLICE_A_ID, FIELD_SLICE_B_ID };
	result = PQexec(_pgConnection, blockConflictsQuery.c_str());

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

		conflictSets->add(conflictSet);
	}

	PQclear(result);

	return conflictSets;
}

bool
PostgreSqlSliceStore::getSlicesFlag(const Block& block) {

	const std::string blockQuery = PostgreSqlUtils::createBlockIdQuery(
				_blockUtils, block, _config.getCatmaidRawStackId());
	std::string blockFlagQuery = "SELECT slices_flag FROM djsopnet_block "
			"WHERE id = (" + blockQuery + ")";
	PGresult* queryResult = PQexec(_pgConnection, blockFlagQuery.c_str());

	PostgreSqlUtils::checkPostgreSqlError(queryResult, blockFlagQuery);

	bool result = 0 == strcmp(PQgetvalue(queryResult, 0, 0), "t");

	PQclear(queryResult);

	return result;
}

void
PostgreSqlSliceStore::saveConnectedComponent(std::string slicePostgreId, const ConnectedComponent& component)
{
	std::ofstream componentFile((_config.getComponentDirectory() + "/" +
			slicePostgreId + ".cmp").c_str());

	// store the component's value
	componentFile << component.getValue() << " ";

	foreach (const util::point<unsigned int>& p, component.getPixels()) {
		componentFile << p.x << " " << p.y << " ";
	}
}

boost::shared_ptr<ConnectedComponent>
PostgreSqlSliceStore::loadConnectedComponent(std::string slicePostgreId)
{

	boost::shared_ptr<Image> source;
	double value;
	boost::shared_ptr<ConnectedComponent::pixel_list_type> pixelList =
			boost::make_shared<ConnectedComponent::pixel_list_type>();
	unsigned int x, y;

	std::ifstream componentFile((_config.getComponentDirectory() + "/" +
			slicePostgreId + ".cmp").c_str());

	// load the component's value
	componentFile >> value;

	while (componentFile.good()) { // TODO: loading in reverse order of original
		componentFile >> x;
		componentFile >> y;
		pixelList->push_back(util::point<unsigned int >(x, y));
	}

	boost::shared_ptr<ConnectedComponent> component = boost::make_shared<ConnectedComponent>(
			source,
			value,
			pixelList,
			0,
			pixelList->size());

	return component;
}

#endif // HAVE_PostgreSQL
