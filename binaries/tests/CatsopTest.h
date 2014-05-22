#ifndef TEST_CATSOP_H__
#define TEST_CATSOP_H__

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

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
	virtual bool run(boost::shared_ptr<T> arg) = 0;
	
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
	class Tester
	{
	public:
		virtual bool runTests() = 0;
	};
	
	template<typename S>
	class TesterImpl : public Tester
	{
	public:
		TesterImpl(const boost::shared_ptr<Test<S> > test,
				   const std::vector<boost::shared_ptr<S> > args);
		bool runTests();
		
	private:
		const boost::shared_ptr<Test<S> > _test;
		const std::vector<boost::shared_ptr<S> >_args;
	};
	
	TestSuite(const std::string& name);
	
	/**
	 * Add a Test to this TestSuite. Tests are executed in the order in which they are
	 * added.
	 * @param test the test to run
	 * @param args the arguments used to run the tests
	 */
	template <typename S>
	void addTest(const boost::shared_ptr<Test<S> > test,
				 const std::vector<boost::shared_ptr<S> > args)
	{
		boost::shared_ptr<Tester> tester = boost::make_shared<TesterImpl<S> >(test, args);
		_testers.push_back(tester);
	}
	
	/**
	 * Run all tests.
	 * @return true if all tests pass, false when one fails.
	 */
	bool runAll();
	
private:
	std::string _name;
	std::vector<boost::shared_ptr<Tester> > _testers;

	
};


};

#endif //TEST_CATSOP_H__
