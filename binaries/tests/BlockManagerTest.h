#ifndef TEST_BLOCK_MANAGER_H__
#define TEST_BLOCK_MANAGER_H__
#include "CatsopTest.h"
#include <sopnet/block/BlockManager.h>
#include <boost/shared_ptr.hpp>
#include <util/point3.hpp>
#include <iostream>

namespace catsoptest
{
class BlockManagerTest : public catsoptest::Test
{
public:
	
	BlockManagerTest(const boost::shared_ptr<BlockManager> blockManager);
	
	bool test();
	
	std::string name();
	
	std::string reason();
	
private:
	boost::shared_ptr<BlockManager> _blockManager;
	std::ostringstream _reason;
	util::point3<unsigned int> _stackSize, _blockSize, _coreSizeInBlocks;
};

};

#endif //TEST_BLOCK_MANAGER_H__