#ifndef TEST_CATSOP_H__
#define TEST_CATSOP_H__

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <util/foreach.h>
#include <util/Logger.h>


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
	class Tester;
private:
	std::string _name;
	std::vector<boost::shared_ptr<Tester> > _testers;
	
	static void logUser(std::string msg);
	static void logError(std::string msg);
	static void logDebug(std::string msg);

	
public:
	
	class Tester
	{
	public:
		virtual bool runTests() = 0;
	};
	
	template<typename S>
	class TesterImpl : public Tester
	{
	private:
		const boost::shared_ptr<Test<S> > _test;
		const std::vector<boost::shared_ptr<S> >_args;
		const std::string _suiteName;
		
	public:
		TesterImpl(const boost::shared_ptr<Test<S> > test,
				   const std::vector<boost::shared_ptr<S> > args,
				   const std::string& suiteName) :
				   _test(test), _args(args), _suiteName(suiteName) {}
		
		bool runTests()
		{
			bool ok = true;
			unsigned int passCount = 0;
			std::stringstream passStr;
			foreach (boost::shared_ptr<S> arg, _args)
			{
				std::stringstream logStr;
				logStr << "Running test "<< _test->name() << " with argument: " << *arg <<
					std::endl;
				TestSuite::logDebug(logStr.str());
				logStr.clear();
				
				if (!_test->run(arg))
				{
					logStr << _suiteName << ": " << _test->name() << ", " << *arg << " FAIL";
					logStr << "\t" << _test->reason();
					logStr << std::endl;
					TestSuite::logError(logStr.str());
					ok = false;
				}
				else
				{
					
					logStr << _suiteName << ": " << _test->name() << ", " << *arg << " Pass";
					TestSuite::logUser(logStr.str());
					++passCount;
				}
			}

			passStr << _suiteName << ": " << passCount << "/" << _args.size() << " passed." <<
				std::endl;
			TestSuite::logUser(passStr.str());
			
			return ok;
		}
		
		
	
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
		boost::shared_ptr<Tester> tester = boost::make_shared<TesterImpl<S> >(test, args, _name);
		_testers.push_back(tester);
	}
	
	/**
	 * Run all tests.
	 * @return true if all tests pass, false when one fails.
	 */
	bool runAll();
	

	
};


};

#endif //TEST_CATSOP_H__
