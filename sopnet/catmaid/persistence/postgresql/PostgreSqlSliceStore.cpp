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
		const boost::shared_ptr<DjangoBlockManager> blockManager,
		const std::string& componentDirectory,
		const std::string& pgHost,
		const std::string& pgUser,
		const std::string& pgPassword,
		const std::string& pgDatabaseName) {
	std::string connectionInfo =
			(pgHost.empty()			? "" : "host="	   + pgHost			+ " ") +
			(pgDatabaseName.empty() ? "" : "dbname="   + pgDatabaseName + " ") +
			(pgUser.empty()			? "" : "user="	   + pgUser			+ " ") +
			(pgPassword.empty()		? "" : "password=" + pgPassword		+ " ");

	_pgConnection = PQconnectdb(connectionInfo.c_str());

	/* Check to see that the backend connection was successfully made */
	if (PQstatus(_pgConnection) != CONNECTION_OK) {

		UTIL_THROW_EXCEPTION(
				PostgreSqlException,
				"Connection to database failed: " << PQerrorMessage(_pgConnection));
	}
}

PostgreSqlSliceStore::~PostgreSqlSliceStore() {

	if (_pgConnection != 0)
		PQfinish(_pgConnection);
}

void
PostgreSqlSliceStore::associateSlicesToBlock(const Slices& slices, const Block& block) {

	if (slices.size() == 0)
		return;

	foreach (boost::shared_ptr<Slice> slice, slices)
	{
		std::string hash = getHash(*slice);
		util::point<double> ctr = slice->getComponent()->getCenter();

		// Make sure that the slice is in the id slice map
		putSlice(slice, hash);

		// Store pixel data of slice
		saveConnectedComponent(hash, *slice->getComponent());

		// Bounding Box
		const util::rect<unsigned int>& bb = slice->getComponent()->getBoundingBox();

		// Create slices in slice table
		// TODO: Use upsert statement, based on CTEs
		std::ostringstream query;
		query << "INSERT INTO djsopnet_slice ";
		query << "(stack_id, section, min_x, min_y, max_x, max_y, ctr_x, " <<
				"ctr_y, value, size, id) VALUES (";
		query << _stack << "," << slice->getSection() << ",";
		query << bb.minX << "," << bb.minY << ",";
		query << bb.maxX << "," << bb.maxY << ",";
		query << ctr.x << "," << ctr.y << ",";
		query << slice->getComponent()->getValue() << ",";
		query << slice->getComponent()->getSize() << ",";
		query << hash << ")";

		PGresult *result = PQexec(_pgConnection, query.str().c_str());
		PostgreSqlUtils::checkPostgreSqlError(result, query.str());
	}

	// store associations with block
}

#endif // HAVE_PostgreSQL
