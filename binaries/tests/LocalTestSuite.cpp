#include "LocalTestSuite.h"
#include <sopnet/block/LocalBlockManager.h>
#include <catmaid/persistence/local/LocalSliceStore.h>
#include <catmaid/persistence/local/LocalSegmentStore.h>
#include <catmaid/persistence/StackStore.h>
#include <catmaid/persistence/local/LocalStackStore.h>
#include <util/ProgramOptions.h>

namespace catsoptest
{


util::ProgramOption optionLocalTestMembranesPath(
util::_module = 			"core",
util::_long_name = 			"localTestMembranes",
util::_description_text = 	"Path to membrane image stack",
util::_default_value =		"./membranes");

util::ProgramOption optionLocalTestRawImagesPath(
	util::_module = 			"core",
	util::_long_name = 			"localTestRaw",
	util::_description_text = 	"Path to raw image stack",
	util::_default_value =		"./raw");

LocalBlockManagerFactory::LocalBlockManagerFactory(const util::point3<unsigned int> stackSize) :
	_stackSize(stackSize)
{
}

boost::shared_ptr<BlockManager>
LocalBlockManagerFactory::createBlockManager(const util::point3<unsigned int> blockSize,
											 const util::point3<unsigned int> coreSizeInBlocks)
{
	boost::shared_ptr<BlockManager> manager =
		boost::make_shared<LocalBlockManager>(_stackSize, blockSize, coreSizeInBlocks);
	return manager;
}

boost::shared_ptr<TestSuite>
LocalTestSuite::localTestSuite(const util::point3<unsigned int> stackSize)
{
	boost::shared_ptr<TestSuite> suite = boost::make_shared<TestSuite>("Local");
	
	addBlockManagerTest(suite, stackSize);
	addSliceStoreTest(suite, stackSize);
	addSegmentStoreTest(suite, stackSize);

	return suite;
}

boost::shared_ptr<SliceStore>
LocalSliceStoreFactory::createSliceStore()
{
	boost::shared_ptr<SliceStore> store = boost::make_shared<LocalSliceStore>();
	return store;
}

void LocalTestSuite::addBlockManagerTest(const boost::shared_ptr<TestSuite> suite,
	const util::point3<unsigned int>& stackSize)
{
	boost::shared_ptr<BlockManagerFactory> factory =
		boost::make_shared<LocalBlockManagerFactory>(stackSize);
	
	boost::shared_ptr<Test<BlockManagerTestParam> > test =
		boost::make_shared<BlockManagerTest>(factory);
	suite->addTest<BlockManagerTestParam>(test,
		BlockManagerTest::generateTestParameters(stackSize));
}

void LocalTestSuite::addSegmentStoreTest(const boost::shared_ptr<TestSuite> suite,
										 const util::point3<unsigned int>& stackSize)
{
	std::string membranePath = optionLocalTestMembranesPath.as<std::string>();
	std::string rawPath = optionLocalTestRawImagesPath.as<std::string>();
	
	boost::shared_ptr<BlockManagerFactory> blockManagerFactory =
		boost::make_shared<LocalBlockManagerFactory>(stackSize);
	boost::shared_ptr<SegmentStoreFactory> segmentStoreFactory = 
		boost::make_shared<LocalSegmentStoreFactory>();
	boost::shared_ptr<StackStore> membraneStackStore =
		boost::make_shared<LocalStackStore>(membranePath);
	boost::shared_ptr<StackStore> rawStackStore = boost::make_shared<LocalStackStore>(rawPath);

	boost::shared_ptr<Test<SegmentStoreTestParam> > test =
		boost::make_shared<SegmentStoreTest>(segmentStoreFactory);
	
	suite->addTest<SegmentStoreTestParam>(test,
		SegmentStoreTest::generateTestParameters("local", stackSize, membraneStackStore,
												 rawStackStore, blockManagerFactory));
}



void LocalTestSuite::addSliceStoreTest(const boost::shared_ptr<TestSuite> suite,
		const util::point3<unsigned int>& stackSize)
{
	std::string membranePath = optionLocalTestMembranesPath.as<std::string>();
	boost::shared_ptr<BlockManagerFactory> blockManagerFactory =
		boost::make_shared<LocalBlockManagerFactory>(stackSize);
	boost::shared_ptr<SliceStoreFactory> sliceStoreFactory = 
		boost::make_shared<LocalSliceStoreFactory>();
	boost::shared_ptr<StackStore> membraneStackStore =
		boost::make_shared<LocalStackStore>(membranePath);
	
	boost::shared_ptr<Test<SliceStoreTestParam> > test = 
		boost::make_shared<SliceStoreTest>(sliceStoreFactory);
	
	suite->addTest<SliceStoreTestParam>(test,
		SliceStoreTest::generateTestParameters("local", stackSize,
											   membraneStackStore, blockManagerFactory));
}

boost::shared_ptr<SegmentStore>
LocalSegmentStoreFactory::createSegmentStore()
{
	boost::shared_ptr<SegmentStore> store = boost::make_shared<LocalSegmentStore>();
	return store;
}


};
