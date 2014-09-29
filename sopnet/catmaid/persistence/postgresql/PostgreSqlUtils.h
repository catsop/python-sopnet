#ifndef POSTGRES_UTILS_H__
#define POSTGRES_UTILS_H__

#include "config.h"
#ifdef HAVE_PostgreSQL

#include <iostream>
#include <sstream>
#include <string>
#include <libpq-fe.h>
#include <util/exceptions.h>

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
};

#endif // HAVE_PostgreSQL

#endif //POSTGRES_UTILS_H__
