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

    unsigned int stack_id = _config.getCatmaidRawStackId();

	BlockUtils blockUtils(_config);

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
		PGresult *result = PQexec(_pgConnection, query.c_str());
		PostgreSqlUtils::checkPostgreSqlError(result, query);
		PQclear(result);

		std::string blockQuery = PostgreSqlUtils::createBlockIdQuery(
				_blockUtils, block, _config.getCatmaidRawStackId());

		std::ostringstream q2;
		q2 << "INSERT INTO djsopnet_sliceblockrelation (block_id, slice_id) ";
		q2 << "VALUES ((" << blockQuery << ")," << hash << ")";

		std::string query2 = q2.str();
		result = PQexec(_pgConnection, query2.c_str());
		PostgreSqlUtils::checkPostgreSqlError(result, query2);
		PQclear(result);
	}
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

	std::string blockQuery = PostgreSqlUtils::createBlockIdQuery(
		_blockUtils, blocks, _config.getCatmaidRawStackId());

	std::ostringstream q;
	q << "SELECT * FROM djsopnet_sliceblockrelation sb INNER JOIN djsopnet_stack s ";
	q << "ON sb.stack_id=s.id";
	q << "WHERE block_id IN (" << blockQuery << ")";

	std::string query = q.str();
	std::cout << query << std::endl;
	PGresult *result = PQexec(_pgConnection, query.c_str());
	PostgreSqlUtils::checkPostgreSqlError(result, query);
	PQclear(result);

	return slices;
}

void
PostgreSqlSliceStore::saveConnectedComponent(std::string sliceHash, const ConnectedComponent& component)
{
	std::ofstream componentFile((_config.getComponentDirectory() + "/" +
			sliceHash + ".cmp").c_str());

	// store the component's value
	componentFile << component.getValue() << " ";

	foreach (const util::point<unsigned int>& p, component.getPixels()) {
		componentFile << p.x << " " << p.y << " ";
	}
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


#endif // HAVE_PostgreSQL
