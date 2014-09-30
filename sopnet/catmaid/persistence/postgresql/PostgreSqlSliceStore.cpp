#include "config.h"
#ifdef HAVE_PostgreSQL

#include <fstream>
#include <catmaid/persistence/django/DjangoUtils.h>
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
			(pgHost.empty()         ? "" : "host="     + pgHost         + " ") +
			(pgDatabaseName.empty() ? "" : "dbname="   + pgDatabaseName + " ") +
			(pgUser.empty()         ? "" : "user="     + pgUser         + " ") +
			(pgPassword.empty()     ? "" : "password=" + pgPassword     + " ");

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

#endif // HAVE_PostgreSQL
