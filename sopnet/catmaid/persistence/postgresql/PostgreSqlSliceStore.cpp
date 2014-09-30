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

#endif // HAVE_PostgreSQL
