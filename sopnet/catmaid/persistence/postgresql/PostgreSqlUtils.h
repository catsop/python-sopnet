#ifndef POSTGRES_UTILS_H__
#define POSTGRES_UTILS_H__

#include "config.h"
#ifdef HAVE_PostgreSQL

#include <iostream>
#include <sstream>
#include <string>
#include <libpq-fe.h>
#include <util/exceptions.h>
#include <catmaid/blocks/BlockUtils.h>
#include <catmaid/blocks/Block.h>
#include <catmaid/blocks/Blocks.h>

typedef signed long long PostgreSqlHash;

struct PostgreSqlException : virtual Exception {};

class PostgreSqlUtils
{
public:
	/**
	 * Check the result of an executed PostgreSQL query. If an error is
	 * detected, this function will throw a PostgreSQLException.
	 * @param result the returned result
	 * @param the query used to obtain the result (optional)
	 */
	static void checkPostgreSqlError(const PGresult *result, const std::string query = "");

	/**
	 * Convert from a Sopnet SegmentHash or SliceHash to the representation used
	 * by PostgreSQL. In practice this is a conversion from an unsigned 64-bit
	 * value to the bit-identical 2s-complement signed 64-bit value.
	 * @param  hash a SegmentHash or SliceHash
	 * @return      the hash's representation in PostgreSQL
	 */
	static PostgreSqlHash hashToPostgreSqlId(const std::size_t hash);

	/**
	 * Convert from a PostgreSQL bigint ID to a representation compatabile with
	 * Sopnet SegmentHash or SliceHash. In practice this is a conversion from a
	 * signed 64-bit value to the bit-identical unsigned 64-bit value.
	 * @param  hash the hash's representation in PostgreSQL
	 * @return      a SegmentHash or SliceHash
	 */
	static std::size_t postgreSqlIdToHash(const PostgreSqlHash hash);

	/**
	 * Connect to a PostgreSql server, given a host, a database name, a user
	 * name and a password. Throws a PostgreSqlException if no connection could
	 * be acquired.
	 */
	static PGconn* getConnection(
			const std::string& host,
			const std::string& port,
			const std::string& database,
			const std::string& user,
			const std::string& pass);

	/**
	 * Create a SQL query that selects the block ID for the given block.
	 */
	static std::string createBlockIdQuery(
			const Block& block,
			unsigned int stackId);
	/**
	 * Create a SQL query that selects the block ID for the given blocks.
	 */
	static std::string createBlockIdQuery(
			const Blocks& blocks,
			unsigned int stackId);

	/**
	 * Create a SQL query that selects the core ID for the given core.
	 */
	static std::string createCoreIdQuery(
			const Core& core,
			unsigned int stackId);

	/**
	 * Execute a query that checks whether the specified flag is true for each
	 * block in a set of blocks, collecting blocks for which the flag is false
	 * in missingBlocks. Returns a comma-separated string of IDs for blocks that
	 * were not missing.
	 */
	static std::string checkBlocksFlags(
			const Blocks& blocks,
			unsigned int stsackId,
			std::string flag,
			Blocks& missingBlocks,
			PGconn* connection);
};

#endif // HAVE_PostgreSQL

#endif //POSTGRES_UTILS_H__
