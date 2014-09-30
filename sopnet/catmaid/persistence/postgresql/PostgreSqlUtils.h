#ifndef POSTGRES_UTILS_H__
#define POSTGRES_UTILS_H__

#include "config.h"
#ifdef HAVE_PostgreSQL

#include <iostream>
#include <sstream>
#include <string>
#include <libpq-fe.h>
#include <util/exceptions.h>

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
	 * Connect to a PostgreSql server, given a host, a database name, a user
	 * name and a password. Throws a PostgreSqlException if no connection could
	 * be acquired.
	 */
	static PGconn* getConnection(const std::string& host, const std::string& database,
			const std::string& user, const std::string& pass);

};

#endif // HAVE_PostgreSQL

#endif //POSTGRES_UTILS_H__
