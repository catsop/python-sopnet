#include "config.h"
#ifdef HAVE_PostgreSQL

#include "PostgreSqlUtils.h"
#include <climits>
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

PostgreSqlHash PostgreSqlUtils::hashToPostgreSqlId(const std::size_t hash) {
	// Compiler-independent conversion from unsigned to signed, see:
	// http://stackoverflow.com/questions/13150449

	if (hash <= LLONG_MAX) return static_cast<PostgreSqlHash>(hash);

	if (hash >= LLONG_MIN)
		return static_cast<PostgreSqlHash>(hash - LLONG_MIN) + LLONG_MIN;

	throw hash;
}

PGconn*
PostgreSqlUtils::getConnection(const std::string& host, const std::string& database,
			const std::string& user, const std::string& pass)
{
	std::string connectionInfo =
			(host.empty()     ? "" : "host="     + host     + " ") +
			(database.empty() ? "" : "dbname="   + database + " ") +
			(user.empty()     ? "" : "user="     + user     + " ") +
			(pass.empty()     ? "" : "password=" + pass     + " ");

	PGconn * connection = PQconnectdb(connectionInfo.c_str());

	/* Check to see that the backend connection was successfully made */
	if (PQstatus(connection) != CONNECTION_OK) {

		UTIL_THROW_EXCEPTION(
				PostgreSqlException,
				"Connection to database failed: " << PQerrorMessage(connection));
	}

    return connection;
}

#endif //HAVE_PostgreSQL
