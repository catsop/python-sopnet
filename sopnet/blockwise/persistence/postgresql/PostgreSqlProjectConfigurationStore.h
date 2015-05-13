#ifndef SOPNET_BLOCKWISE_PERSISTENCE_POSTGRESQL_POSTGRESQL_PROJECT_CONFIGURATION_STORE_H__
#define SOPNET_BLOCKWISE_PERSISTENCE_POSTGRESQL_POSTGRESQL_PROJECT_CONFIGURATION_STORE_H__


#include "config.h"
#ifdef HAVE_PostgreSQL

#include <blockwise/ProjectConfiguration.h>
#include <libpq-fe.h>

class PostgreSqlProjectConfigurationStore {

public:

	PostgreSqlProjectConfigurationStore(const ProjectConfiguration& configuration);

	~PostgreSqlProjectConfigurationStore();

	void fill(ProjectConfiguration& configuration);

private:

	void fill(StackDescription& stackDescription, StackType stackType);

	// database connection
	PGconn* _pgConnection;
};

#endif // HAVE_PostgreSQL

#endif // SOPNET_BLOCKWISE_PERSISTENCE_POSTGRESQL_POSTGRESQL_PROJECT_CONFIGURATION_STORE_H__

