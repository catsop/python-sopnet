#ifndef TEST_LOCAL_SUITE_H__
#define TEST_LOCAL_SUITE_H__
#include "BlockManagerTest.h"
#include "SliceStoreTest.h"
#include "SegmentStoreTest.h"
#include <sopnet/block/BlockManager.h>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <util/point3.hpp>

namespace catsoptest
{

class LocalSliceStoreFactory : public SliceStoreFactory
{
public:
	boost::shared_ptr<SliceStore> createSliceStore();
};

class LocalSegmentStoreFactory : public SegmentStoreFactory
{
public:
	boost::shared_ptr<SegmentStore> createSegmentStore();
};

class LocalBlockManagerFactory : public BlockManagerFactory
{
public:
	LocalBlockManagerFactory(const util::point3<unsigned int> stackSize);
	
	boost::shared_ptr<BlockManager> createBlockManager(
		const util::point3<unsigned int> blockSize,
		const util::point3<unsigned int> coreSizeInBlocks);
	
private:
	const util::point3<unsigned int> _stackSize;
};
	
class LocalTestSuite
{
public:
	static boost::shared_ptr<TestSuite> localTestSuite(const util::point3<unsigned int> stackSize);
private:
	static void addBlockManagerTest(const boost::shared_ptr<TestSuite> suite,
		const util::point3<unsigned int>& stackSize);
	static void addSliceStoreTest(const boost::shared_ptr<TestSuite> suite,
		const util::point3<unsigned int>& stackSize);
	static void addSegmentStoreTest(const boost::shared_ptr<TestSuite> suite,
		const util::point3<unsigned int>& stackSize);
};

};


#endif //TEST_LOCAL_SUITE_H__