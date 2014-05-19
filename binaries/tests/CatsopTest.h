#ifndef TEST_CATSOP_H__
#define TEST_CATSOP_H__

#include <string>

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
	 * @return true for a successfult test, false otherwise.
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

};

#endif //TEST_CATSOP_H__