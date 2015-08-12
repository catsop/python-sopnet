#include "logging.h"

namespace python {

logger::LogChannel pylog("pylog", "[pysopnet] ");

logger::LogLevel getLogLevel() {
	return logger::LogManager::getGlobalLogLevel();
}

void setLogLevel(logger::LogLevel logLevel) {
	logger::LogManager::setGlobalLogLevel(logLevel);
}

}
