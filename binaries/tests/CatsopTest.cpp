#include "CatsopTest.h"
#include <util/foreach.h>
#include <util/Logger.h>

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

template <typename S>
TestSuite::TesterImpl<S>::TesterImpl(const boost::shared_ptr<Test<S> > test,
								const std::vector<boost::shared_ptr<S> > args) :
								  _test(test), _args(args)
{
}

template <typename S>
bool TestSuite::TesterImpl<S>::runTests()
{
	bool ok = true;
	unsigned int passCount = 0;
	foreach (boost::shared_ptr<S> arg, _args)
	{
		if (!_test->run(arg))
		{
			LOG_ERROR(catsoptestlog) << _name << ": " << _test->name() << " FAIL" << std::endl;
			LOG_ERROR(catsoptestlog) << "\t" << _test->reason();
			LOG_ERROR(catsoptestlog) << std::endl << std::endl;
			ok = false;
		}
		else
		{
			LOG_USER(catsoptestlog) << _name << ": " << _test->name() << " Pass" << std::endl;
			++passCount;
		}
	}

	LOG_USER(catsoptestlog) << _name << ": " << passCount << "/" << _args.size() << " passed." <<
		std::endl;
	
	return ok;
}


};