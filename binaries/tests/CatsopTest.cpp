#include "CatsopTest.h"
#include <util/foreach.h>

logger::LogChannel catsoptestlog("catsoptestlog", "[CatsopTest] ");

namespace catsoptest
{

TestSuite::TestSuite(const std::string& name) : _name(name)
{
	LOG_USER(catsoptestlog) << "Initiated test suite " << name << std::endl;
}

bool TestSuite::runAll()
{
	foreach (boost::shared_ptr<Tester> tester, _testers)
	{
		if (!tester->runTests())
		{
			return false;
		}
	}
	
	return true;
}

void TestSuite::logError(std::string msg)
{
	LOG_ERROR(catsoptestlog) << msg << std::endl;
}

void TestSuite::logUser(std::string msg)
{
	LOG_USER(catsoptestlog) << msg << std::endl;
}

void TestSuite::logDebug(std::string msg)
{
	LOG_DEBUG(catsoptestlog) << msg << std::endl;
}



};