#ifndef SOPNET_PYTHON_LOGGING_H__
#define SOPNET_PYTHON_LOGGING_H__

#include <util/Logger.h>

namespace python {

extern logger::LogChannel pylog;

/**
 * Set the log level of the python wrappers:
 *
 * 0	quiet
 * 1	errors
 * 2	user
 * 3	debug
 * 4	all
 */
void setLogLevel(unsigned int level);

} // namespace python

#endif // SOPNET_PYTHON_LOGGING_H__

