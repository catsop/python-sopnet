#include "CatsopTest.h"
#include <boost/foreach.hpp>

namespace catsoptest
{
TestSuite::TestSuite(const std::string& name) : _name(name)
{

}

void
TestSuite::addTest(const boost::shared_ptr<Test<T> > test, const arglist_type& args)
{
	_tests.push_back(test);
	_argumentLists.push_back(args);
}

bool
TestSuite::runAll()
{
	for (unsigned int i = 0; i < _tests.size(); ++i)
	{
		
		
	}
	
	
}



};