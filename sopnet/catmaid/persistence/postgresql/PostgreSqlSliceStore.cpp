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

		std::string blockQuery = PostgreSqlUtils::createBlockIdQuery(_blockUtils, block);

		std::ostringstream q2;
		q2 << "INSERT INTO djsopnet_sliceblockrelation (block_id, slice_id) ";
		q2 << "VALUES ((" << blockQuery << ")," << hash << ")";

		std::string query2 = q2.str();
		result = PQexec(_pgConnection, query2.c_str());
		PostgreSqlUtils::checkPostgreSqlError(result, query2);
	}
}

void
PostgreSqlSliceStore::associateConflictSetsToBlock(
		const ConflictSets& conflictSets, const Block& block)
{
	if (conflictSets.size() == 0)
		return;

	std::string blockQuery = PostgreSqlUtils::createBlockIdQuery(_blockUtils, block);

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
					q << "INSERT INTO djsopnet_sliceconflictset ";
					q << "(slice_a_id, slice_b_id) VALUES ";
					q << "(" << hash1 << "," << hash2 << "); ";

					// Associate conflict set to block
					q << "INSERT INTO djsopnet_blockconflictrelation ";
					q << "(block_id, conflict_id) VALUES ";
					q << "((" << blockQuery << "),";
					q << "currval('djsopnet_blockconflictrelation_id_seq'))";

					std::string query = q.str();
					PGresult *result = PQexec(_pgConnection, query.c_str());
					PostgreSqlUtils::checkPostgreSqlError(result, query);
				}
			}
		}
	}
}

bool
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


#endif // HAVE_PostgreSQL
