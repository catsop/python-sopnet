#ifndef TEST_BLOCK_MANAGER_H__
#define TEST_BLOCK_MANAGER_H__
#include "CatsopTest.h"
#include <sopnet/block/BlockManager.h>
#include <boost/shared_ptr.hpp>
#include <util/point3.hpp>
#include <iostream>

namespace catsoptest
{
	
class BlockManagerFactory
{
public:
	virtual boost::shared_ptr<BlockManager> createBlockManager(
		const util::point3<unsigned int> blockSize,
		const util::point3<unsigned int> coreSizeInBlocks) = 0;
};
	
	
class BlockManagerTestParam
{
public:
	BlockManagerTestParam();
	
	BlockManagerTestParam(const util::point3<unsigned int> bs,
						  const util::point3<unsigned int> cs) :
						  blockSize(bs), coreSizeInBlocks(cs) {}

	util::point3<unsigned int> blockSize;
	util::point3<unsigned int> coreSizeInBlocks;
};

class BlockManagerTest : public catsoptest::Test<BlockManagerTestParam>
{
public:
	
	BlockManagerTest(const boost::shared_ptr<BlockManagerFactory> blockManagerFactory);
	
	bool run(boost::shared_ptr<BlockManagerTestParam> arg);
	
	std::string name();
	
	std::string reason();
	
	static std::vector<boost::shared_ptr<BlockManagerTestParam> >
		generateTestParameters(const util::point3<unsigned int>& stackSize);
	
	
private:
	const boost::shared_ptr<BlockManagerFactory> _blockManagerFactory;
	std::ostringstream _reason;
};

};

std::ostream& operator<<(std::ostream& os, const catsoptest::BlockManagerTestParam& param);

#endif //TEST_BLOCK_MANAGER_H__