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

template <typename T>
class Test
{
public:
	/**
	 * Run this test over the given argument.
	 * @param arg an argument 
	 * @return true for a successful test, false otherwise.
	 */
	virtual bool run(T arg) = 0;
	
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
	 * @param test the test to run
	 * @param args the arguments used to run the tests
	 */
	template <typename S>
	void addTest(const boost::shared_ptr<Test<S> > test, const std::vector<S> args);
	
	/**
	 * Run all tests.
	 * @return true if all tests pass, false when one fails.
	 */
	bool runAll();
	
private:
	template<typename S>
	class Tester
	{
	public:
		Tester(const boost::shared_ptr<Test<S> > test, const std::vector<S> args);
		
	private:
		const boost::shared_ptr<Test<S> > _test;
		const std::vector<S> _args;
	};
	
	std::string _name;
	std::vector<boost::shared_ptr<Test<T> > > _tests;
	std::vector<arglist_type> _argumentLists;
};

};

#endif //TEST_CATSOP_H__
