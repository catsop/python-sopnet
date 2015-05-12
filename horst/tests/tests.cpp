#include "tests.h"

logger::LogChannel testslog("testslog", "[tests] ");

boost::filesystem::path dir_of(const char* filename) {

	boost::filesystem::path filepath(filename);
	return filepath.parent_path();
}
