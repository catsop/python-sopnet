#ifndef TEST_CATSOP_H__
#define TEST_CATSOP_H__

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

/**
 * A really simple unit test designed for the catsop project.
 */

namespace catsoptest
{

class Test
{
public:
	/**
	 * Run this test.
	 * @return true for a successful test, false otherwise.
	 */
	virtual bool test() = 0;
	
	/**
	 * @return the name of this test
	 */
	virtual std::string name() = 0;
	
	/**
	 * @return a string describing the reason for a failure, if one occurred.
	 */
	virtual std::string reason() = 0;
};

//TODO: consider a dependency graph implementation. Use sopnet pipeline if this is necessary.

class TestSuite
{
public:
	TestSuite(const std::string& name);
	
	/**
	 * Add a Test to this TestSuite. Tests are executed in the order in which they are
	 * added.
	 */
	void addTest(const boost::shared_ptr<Test> test);
	
	/**
	 * Run all tests.
	 * @return true if all tests pass, false when one fails.
	 */
	bool runAll();
	
private:
	std::string _name;
	std::vector<boost::shared_ptr<Test> > _tests;
};

};

#endif //TEST_CATSOP_H__
