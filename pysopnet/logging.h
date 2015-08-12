#ifndef SOPNET_PYTHON_LOGGING_H__
#define SOPNET_PYTHON_LOGGING_H__

#include <util/Logger.h>

namespace python {

extern logger::LogChannel pylog;

/**
 * Get the log level of the python wrappers.
 */
logger::LogLevel getLogLevel();

/**
 * Set the log level of the python wrappers.
 */
void setLogLevel(logger::LogLevel logLevel);

} // namespace python

#endif // SOPNET_PYTHON_LOGGING_H__

