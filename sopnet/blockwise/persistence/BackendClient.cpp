#include "config.h"
#include <blockwise/persistence/catmaid/CatmaidStackStore.h>
#ifdef HAVE_PostgreSQL
#include <blockwise/persistence/postgresql/PostgreSqlSliceStore.h>
#include <blockwise/persistence/postgresql/PostgreSqlSegmentStore.h>
#endif
#include <blockwise/persistence/local/LocalStackStore.h>
#include <blockwise/persistence/local/LocalSliceStore.h>
#include <blockwise/persistence/local/LocalSegmentStore.h>
#include "BackendClient.h"
#include "logging.h"

namespace python {

boost::shared_ptr<StackStore>
BackendClient::createStackStore(const ProjectConfiguration& configuration, StackType type) {

	if (configuration.getBackendType() == ProjectConfiguration::Local) {

		LOG_USER(pylog) << "[BackendClient] create local stack store for membranes" << std::endl;

		return boost::make_shared<LocalStackStore>(type == Raw ? "./raw" : "./membranes");
	}

	if (configuration.getBackendType() == ProjectConfiguration::PostgreSql) {

		LOG_USER(pylog) << "[BackendClient] create catmaid stack store for membranes" << std::endl;

		return boost::make_shared<CatmaidStackStore>(configuration, type);
	}

	UTIL_THROW_EXCEPTION(UsageError, "unknown backend type " << configuration.getBackendType());
}

boost::shared_ptr<SliceStore>
BackendClient::createSliceStore(const ProjectConfiguration& configuration) {

	if (configuration.getBackendType() == ProjectConfiguration::Local) {

		LOG_USER(pylog) << "[BackendClient] create local slice store" << std::endl;

		return boost::make_shared<LocalSliceStore>();
	}

#ifdef HAVE_PostgreSQL
	if (configuration.getBackendType() == ProjectConfiguration::PostgreSql) {

		LOG_USER(pylog) << "[BackendClient] create postgresql slice store" << std::endl;

		return boost::make_shared<PostgreSqlSliceStore>(configuration);
	}
#endif // HAVE_PostgreSQL

	UTIL_THROW_EXCEPTION(UsageError, "unknown backend type " << configuration.getBackendType());
}

boost::shared_ptr<SegmentStore>
BackendClient::createSegmentStore(const ProjectConfiguration& configuration) {

	if (configuration.getBackendType() == ProjectConfiguration::Local) {

		LOG_USER(pylog) << "[BackendClient] create local segment store" << std::endl;

		return boost::make_shared<LocalSegmentStore>(configuration);
	}

#ifdef HAVE_PostgreSQL
	if (configuration.getBackendType() == ProjectConfiguration::PostgreSql) {

		LOG_USER(pylog) << "[BackendClient] create postgresql segment store" << std::endl;

		return boost::make_shared<PostgreSqlSegmentStore>(configuration);
	}
#endif // HAVE_PostgreSQL

	UTIL_THROW_EXCEPTION(UsageError, "unknown backend type " << configuration.getBackendType());
}

} // namespace python
