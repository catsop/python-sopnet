#include "CatsopTest.h"
#include <util/foreach.h>

logger::LogChannel catsoptestlog("catsoptestlog", "[CatsopTest] ");

namespace catsoptest
{

TestSuite::TestSuite(const std::string& name) : _name(name)
{
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
	LOG_ERROR(catsoptestlog) << msg;
}

void TestSuite::logUser(std::string msg)
{
	LOG_USER(catsoptestlog) << msg;
}



};