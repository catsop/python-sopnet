#include "config.h"
#ifdef HAVE_PostgreSQL

#include "PostgreSqlUtils.h"
#include <fstream>
#include <imageprocessing/ConnectedComponent.h>
#include <imageprocessing/Image.h>
#include <sopnet/slices/ComponentTreeConverter.h>
#include <util/ProgramOptions.h>
#include <util/httpclient.h>
#include <util/point.hpp>
#include "PostgreSqlSliceStore.h"

logger::LogChannel postgresqlslicestorelog("postgresqlslicestorelog", "[PostgreSqlSliceStore] ");

PostgreSqlSliceStore::PostgreSqlSliceStore(
        const ProjectConfiguration& config) : _config(config)
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

		std::ostringstream q2;
		q2 << "INSERT INTO djsopnet_sliceblockrelation (block_id, slice_id) ";
		q2 << "VALUES (" << block.getId() << "," << hash << ")";

		std::string query2 = q2.str();
		result = PQexec(_pgConnection, query2.c_str());
		PostgreSqlUtils::checkPostgreSqlError(result, query2);
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
