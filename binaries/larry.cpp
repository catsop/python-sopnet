
#include <iostream>
#include <string>
#include <fstream>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <util/Logger.h>

#include <pipeline/all.h>
#include <pipeline/Value.h>

#include <boost/unordered_set.hpp>
#include <util/ProgramOptions.h>
#include <util/exceptions.h>
#include <util/httpclient.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <sopnet/catmaid/django/DjangoBlockManager.h>
#include <tests/BlockManagerTest.h>
#include <tests/DjangoTestSuite.h>
#include <tests/LocalTestSuite.h>
#include <tests/CatsopTest.h>

using std::cout;
using std::endl;
using namespace logger;

void handleException(boost::exception& e) {

	LOG_ERROR(out) << "[window thread] caught exception: ";

	if (boost::get_error_info<error_message>(e))
		LOG_ERROR(out) << *boost::get_error_info<error_message>(e);

	if (boost::get_error_info<stack_trace>(e))
		LOG_ERROR(out) << *boost::get_error_info<stack_trace>(e);

	LOG_ERROR(out) << std::endl;

	LOG_ERROR(out) << "[window thread] details: " << std::endl
	               << boost::diagnostic_information(e)
	               << std::endl;

	exit(-1);
}

int main(int optionc, char** optionv)
{
	util::ProgramOptions::init(optionc, optionv);
	LogManager::init();
	
	try
	{
		boost::shared_ptr<catsoptest::TestSuite> djangoSuite =
			catsoptest::DjangoTestSuite::djangoTestSuite("catmaid:8000", 4, 3);
		djangoSuite->runAll();
// 		boost::shared_ptr<catsoptest::TestSuite> localSuite = 
// 			catsoptest::LocalTestSuite::localTestSuite(util::point3<unsigned int>(179, 168, 5));
// 		localSuite->runAll();
		
	}
	catch (Exception& e)
	{
		handleException(e);
	}
}
