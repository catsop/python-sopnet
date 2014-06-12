#include "SegmentStoreTest.h"
#include <sopnet/catmaid/persistence/LocalSliceStore.h>
#include <sopnet/catmaid/persistence/LocalSegmentStore.h>
#include <sopnet/catmaid/SegmentGuarantor.h>
#include <sopnet/catmaid/persistence/SegmentReader.h>
#include <sopnet/catmaid/persistence/CostReader.h>
#include <sopnet/catmaid/persistence/CostWriter.h>
#include <sopnet/catmaid/persistence/SolutionReader.h>
#include <sopnet/catmaid/persistence/SolutionWriter.h>
#include <sopnet/catmaid/persistence/SegmentFeatureReader.h>
#include <sopnet/catmaid/persistence/SegmentWriter.h>

namespace catsoptest
{
	//TODO: consider chaining with SliceStoreTest

SegmentStoreTestParam::SegmentStoreTestParam(const std::string& inName,
	const boost::shared_ptr<StackStore> inMembraneStackStore,
	const boost::shared_ptr<StackStore> inRawStackStore,
	const boost::shared_ptr<BlockManagerFactory> blockManagerFactory,
	const boost::shared_ptr< BlockManagerTestParam > blockManagerArg):
	name(inName), membraneStackStore(inMembraneStackStore),
	rawStackStore(inRawStackStore), _factory(blockManagerFactory),
	_blockManagerParam(blockManagerArg)
{
	
}

boost::shared_ptr<BlockManager>
SegmentStoreTestParam::blockManager()
{
	return _factory->createBlockManager(_blockManagerParam->blockSize,
		_blockManagerParam->coreSizeInBlocks);
}

boost::shared_ptr<BlockManagerTestParam>
SegmentStoreTestParam::getBlockManagerParam() const
{
	return _blockManagerParam;
}

SegmentStoreTest::SegmentStoreTest(const boost::shared_ptr<SegmentStoreFactory> factory):
	_factory(factory)
{
	
}

bool
SegmentStoreTest::run(boost::shared_ptr<SegmentStoreTestParam> arg)
{
	boost::shared_ptr<BlockManager> blockManager = arg->blockManager();
	boost::shared_ptr<SliceStore> sliceStore = boost::make_shared<LocalSliceStore>();
	boost::shared_ptr<SegmentStore> segmentStore = boost::make_shared<LocalSegmentStore>();
	boost::shared_ptr<SegmentStore> testSegmentStore = _factory->createSegmentStore();
	
	guaranteeSlices(sliceStore, arg->membraneStackStore, blockManager);
	guaranteeSegments(segmentStore, sliceStore, arg->rawStackStore, blockManager);
	copyStores(segmentStore, testSegmentStore, blockManager);
	return verifyStores(segmentStore, testSegmentStore, blockManager);
}


void
SegmentStoreTest::guaranteeSlices(const boost::shared_ptr<SliceStore> sliceStore,
								  const boost::shared_ptr<StackStore> stackStore,
								  const boost::shared_ptr<BlockManager> blockManager)
{
	boost::shared_ptr<SliceGuarantor> guarantor = boost::make_shared<SliceGuarantor>();
	boost::shared_ptr<Box<> > box = boost::make_shared<Box<> >(util::point3<unsigned int>(0,0,0),
															   blockManager->stackSize());
	boost::shared_ptr<Blocks> blocks = blockManager->blocksInBox(box);
	
	guarantor->setInput("blocks", blocks);
	guarantor->setInput("slice store", sliceStore);
	guarantor->setInput("stack store", stackStore);
	
	guarantor->guaranteeSlices();
}

void
SegmentStoreTest::guaranteeSegments(const boost::shared_ptr<SegmentStore> segmentStore,
									const boost::shared_ptr<SliceStore> sliceStore,
									const boost::shared_ptr<StackStore> stackStore,
									const boost::shared_ptr<BlockManager> blockManager)
{
	boost::shared_ptr<SegmentGuarantor> guarantor = boost::make_shared<SegmentGuarantor>();
	boost::shared_ptr<Box<> > box = boost::make_shared<Box<> >(util::point3<unsigned int>(0,0,0),
															   blockManager->stackSize());
	boost::shared_ptr<Blocks> blocks = blockManager->blocksInBox(box);
	
	guarantor->setInput("blocks", blocks);
	guarantor->setInput("slice store", sliceStore);
	guarantor->setInput("segment store", segmentStore);
	guarantor->setInput("stack store", stackStore);
	
	guarantor->guaranteeSegments();
}


void
SegmentStoreTest::copyStores(const boost::shared_ptr<SegmentStore> store,
							 const boost::shared_ptr<SegmentStore> testStore,
							 const boost::shared_ptr<BlockManager> blockManager)
{
	boost::shared_ptr<Box<> > box = boost::make_shared<Box<> >(util::point3<unsigned int>(0,0,0),
															   blockManager->stackSize());
	boost::shared_ptr<Blocks> blocks = blockManager->blocksInBox(box);
	
	boost::shared_ptr<CostReader> costReader = boost::make_shared<CostReader>();
	boost::shared_ptr<CostWriter> costWriter = boost::make_shared<CostWriter>();
	boost::shared_ptr<SolutionReader> solutionReader = boost::make_shared<SolutionReader>();
	boost::shared_ptr<SolutionWriter> solutionWriter = boost::make_shared<SolutionWriter>();
	
	boost::shared_ptr<SegmentFeatureReader> featureReader = boost::make_shared<SegmentFeatureReader>();
	
	boost::shared_ptr<SegmentReader> reader = boost::make_shared<SegmentReader>();
	boost::shared_ptr<SegmentWriter> writer = boost::make_shared<SegmentWriter>();
	
	boost::shared_ptr<bool> yeah = boost::make_shared<bool>(true);
	
	reader->setInput("blocks", blocks);
	reader->setInput("store", store);
	
	featureReader->setInput("segments", reader->getOutput("segments"));
	featureReader->setInput("store", store);
	featureReader->setInput("stored only", yeah);
	
	costReader->setInput("store", store);
	costReader->setInput("segments", reader->getOutput("segments"));
	
	writer->setInput("blocks", blocks);
	writer->setInput("store", testStore);
	writer->setInput("segments", reader->getOutput("segments"));
	writer->setInput("features", featureReader->getOutput());
	
	costWriter->setInput("store", store);
	costWriter->setInput("segments", reader->getOutput("segments"));
	costWriter->setInput("objective", costReader->getOutput());
	
	writer->writeSegments();
	costWriter->writeCosts();
}



};