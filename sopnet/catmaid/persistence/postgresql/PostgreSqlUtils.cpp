#include "config.h"
#ifdef HAVE_PostgreSQL

#include "PostgreSqlUtils.h"
#include <util/Logger.h>

logger::LogChannel postgresqlutilslog("postgresqlutilslog", "[PostgreSqlUtils] ");

void
PostgreSqlUtils::checkPostgreSqlError(const PGresult *result, const std::string query)
{
	ExecStatusType status = PQresultStatus(result);
	if (PGRES_COMMAND_OK != status || PGRES_TUPLES_OK != status) {
		LOG_ERROR(postgresqlutilslog) << "Unexpected result: " <<
				PQresStatus(status) << std::endl <<
				PQresultErrorMessage(result) << std::endl;
		LOG_ERROR(postgresqlutilslog) << "The used query was: " << query <<
			std::endl;
		UTIL_THROW_EXCEPTION(
			PostgreSqlException,
			"A PostgreSQL query returned an unexpected result (" <<
					PQresStatus(status) << ").");
	}
}

#endif //HAVE_PostgreSQL
