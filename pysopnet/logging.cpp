#include "logging.h"

namespace python {

logger::LogChannel pylog("pylog", "[pysopnet] ");

void setLogLevel(unsigned int level) {

	logger::LogLevel logLevel;

	switch (level) {

		case 0:
			logLevel = logger::Quiet;
			break;

		case 1:
			logLevel = logger::Error;
			break;

		case 3:
			logLevel = logger::Debug;
			break;

		case 4:
			logLevel = logger::All;
			break;

		default:
			logLevel = logger::User;
			break;
	}

	logger::LogManager::setGlobalLogLevel(logLevel);
}

}
