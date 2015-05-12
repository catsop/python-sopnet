#include <iostream>
#include <tests.h>
#include <util/ProgramOptions.h>
#include <util/Logger.h>
#include <util/exceptions.h>

BEGIN_TEST_MODULE(host)

	ADD_TEST_SUITE(graph);
	ADD_TEST_SUITE(conflict_candidates);
	ADD_TEST_SUITE(multi_factors);
	ADD_TEST_SUITE(gap);
	ADD_TEST_SUITE(imageprocessing);

END_TEST_MODULE()

void exceptionTranslator(const Exception& error) {

	handleException(error, std::cout);
}

int main(int argc, char** argv) {

	util::ProgramOptions::init(argc, argv, true);
	logger::LogManager::init();

	::boost::unit_test::unit_test_monitor.register_exception_translator<Exception>(&exceptionTranslator);

	return ::boost::unit_test::unit_test_main(&host, argc, argv);
}
