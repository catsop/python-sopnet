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
PostgreSqlUtils::getConnection(
		const std::string& host,
		const std::string& port,
		const std::string& database,
		const std::string& user,
		const std::string& pass)
{
	std::string connectionInfo =
			(host.empty()     ? "" : "host="     + host     + " ") +
			(port.empty()     ? "" : "port="     + port     + " ") +
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
PostgreSqlUtils::createBlockIdQuery(
	const Block& block,
	unsigned int stackId)
{
	std::ostringstream blockQuery;
	blockQuery << "SELECT id FROM djsopnet_block WHERE ";
	blockQuery << "stack_id=" << stackId << " AND ";
	blockQuery << "coordinate_x=" << block.x() << " AND ";
	blockQuery << "coordinate_y=" << block.y() << " AND ";
	blockQuery << "coordinate_z=" << block.z() << " LIMIT 1";

    return blockQuery.str();
}

std::string
PostgreSqlUtils::createBlockIdQuery(
	const Blocks& blocks,
	unsigned int stackId)
{
	std::string delim = "";
	std::ostringstream blockQuery;
	blockQuery << "SELECT id FROM djsopnet_block WHERE ";
	blockQuery << "stack_id=" << stackId << "AND ";
	foreach (const Block &block, blocks)
	{
		blockQuery << delim;
		blockQuery << "(coordinate_x=" << block.x() << " AND ";
		blockQuery << "coordinate_y=" << block.y() << " AND ";
		blockQuery << "coordinate_z=" << block.z() << ")";
		delim = " OR ";
	}

    return blockQuery.str();
}

std::string
PostgreSqlUtils::createCoreIdQuery(
	const Core& core,
	unsigned int stackId)
{
	std::ostringstream blockQuery;
	blockQuery << "SELECT id FROM djsopnet_core WHERE ";
	blockQuery << "stack_id=" << stackId << "AND ";
	blockQuery << "coordinate_x=" << core.x() << " AND ";
	blockQuery << "coordinate_y=" << core.y() << " AND ";
	blockQuery << "coordinate_z=" << core.z() << " LIMIT 1";

    return blockQuery.str();
}

std::string
PostgreSqlUtils::checkBlocksFlags(
		const Blocks& blocks,
		unsigned int stackId,
		std::string flag,
		Blocks& missingBlocks,
		PGconn* connection) {

	std::ostringstream blockQuery;
	blockQuery << "WITH block_coords AS (VALUES";
	char separator = ' ';
	unsigned int blockIndex = 0;

	foreach (const Block& block, blocks) {
		blockQuery << separator << '('
				<< blockIndex++ << ','
				<< block.x() << ','
				<< block.y() << ','
				<< block.z() << ')';
		separator = ',';
	}

	blockQuery << ") SELECT bm.id, b.id, b." << flag
			<< " FROM block_coords AS bm (id,coord_x,coord_y,coord_z) "
				"LEFT JOIN djsopnet_block b ON (b.stack_id = " << stackId << " AND "
				"b.coordinate_x = bm.coord_x AND "
				"b.coordinate_y = bm.coord_y AND "
				"b.coordinate_z = bm.coord_z) ORDER BY bm.id ASC;";
	enum { FIELD_INDEX, FIELD_ID, FIELD_FLAG };
	std::string query = blockQuery.str();

	PGresult* queryResult = PQexec(connection, query.c_str());
	checkPostgreSqlError(queryResult, query);

	unsigned int nBlocks = boost::numeric_cast<unsigned int>(PQntuples(queryResult));

	if (nBlocks != blocks.size()) {
		std::ostringstream errorMsg;
		errorMsg << "Wrong number of block rows returned, expected " << blocks.size()
				<< ", received " << nBlocks << ". The used query was: " << query << std::endl;
		LOG_ERROR(postgresqlutilslog) << errorMsg.str();
		UTIL_THROW_EXCEPTION(
			PostgreSqlException,
			"A PostgreSQL query returned an unexpected result (" << errorMsg.str() << ").");
	}

	std::ostringstream blockIds;
	int i = 0;
	separator = ' ';

	// Since Blocks uses a std::set we are guaranteed the same iteration order as above.
	foreach (const Block& block, blocks) {
		if (0 != strcmp(PQgetvalue(queryResult, i, FIELD_FLAG), "t")) {
			missingBlocks.add(block);
		} else {
			blockIds << separator << PQgetvalue(queryResult, i, FIELD_ID);
			separator = ',';
		}

		i++;
	}

	PQclear(queryResult);

	return blockIds.str();
}

#endif //HAVE_PostgreSQL
