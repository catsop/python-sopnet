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
	if (PGRES_COMMAND_OK != status && PGRES_TUPLES_OK != status) {
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

PostgreSqlHash
PostgreSqlUtils::hashToPostgreSqlId(const std::size_t hash) {
	// Compiler-independent conversion from unsigned to signed, see:
	// http://stackoverflow.com/questions/13150449

	if (hash <= LLONG_MAX) return static_cast<PostgreSqlHash>(hash);

	if (hash >= LLONG_MIN)
		return static_cast<PostgreSqlHash>(hash - LLONG_MIN) + LLONG_MIN;

	throw hash;
}

std::size_t
PostgreSqlUtils::postgreSqlIdToHash(const PostgreSqlHash pgId) {
	if (pgId >= 0) return static_cast<std::size_t>(pgId);

	if (pgId <= ULLONG_MAX)
		return static_cast<PostgreSqlHash>(pgId + ULLONG_MAX) - ULLONG_MAX;
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

std::string
PostgreSqlUtils::createBlockIdQuery(const BlockUtils& blockUtils, const Block& block)
{
	util::box<unsigned int> boxBb = blockUtils.getBoundingBox(block);
	std::ostringstream blockQuery;
	blockQuery << "SELECT id FROM djsopnet_block WHERE ";
	blockQuery << "min_x=" << boxBb.min.x << "AND ";
	blockQuery << "min_y=" << boxBb.min.y << "AND ";
	blockQuery << "min_z=" << boxBb.min.z << "LIMIT 1";

    return blockQuery.str();
}

std::string
PostgreSqlUtils::createBlockIdQuery(const BlockUtils& blockUtils, const Blocks& blocks)
{
	std::string delim = "";
	std::ostringstream blockQuery;
	blockQuery << "SELECT id FROM djsopnet_block WHERE ";
	foreach (const Block &block, blocks)
	{
		util::box<unsigned int> bb = blockUtils.getBoundingBox(block);
		blockQuery << delim;
		blockQuery << "(min_x=" << bb.min.x << "AND ";
		blockQuery << "min_y=" << bb.min.y << "AND ";
		blockQuery << "min_z=" << bb.min.z << ")";
		delim = " OR ";
	}

    return blockQuery.str();
}

#endif //HAVE_PostgreSQL
