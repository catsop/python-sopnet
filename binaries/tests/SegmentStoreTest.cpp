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
#include <sopnet/inference/Reconstructor.h>
#include <sopnet/catmaid/persistence/SegmentPointerHash.h>
#include <util/Logger.h>

namespace catsoptest
{
	//TODO: consider chaining with SliceStoreTest

logger::LogChannel segmentstoretestlog("segmentstoretestlog", "[SegmentStoreTest] ");
	
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
	
	costWriter->setInput("store", testStore);
	costWriter->setInput("segments", reader->getOutput("segments"));
	costWriter->setInput("objective", costReader->getOutput());
	
	writer->writeSegments();
	costWriter->writeCosts();
	
	solutionReader->setInput("store", store);
	solutionWriter->setInput("store", testStore);
	
	foreach (boost::shared_ptr<Core> core, blockManager->coresInBox(box))
	{
		boost::shared_ptr<Cores> cores = boost::make_shared<Cores>();
		cores->add(core);
		reader->setInput("blocks", core);
		solutionReader->setInput("core", core);
		solutionReader->setInput("segments", reader->getOutput("segments"));
		solutionWriter->setInput("segments", reader->getOutput("segments"));
		solutionWriter->setInput("cores", cores);
		solutionWriter->setInput("solution", solutionReader->getOutput());
		
		solutionWriter->writeSolution();
	}
	
}

bool
SegmentStoreTest::costEqual(const boost::shared_ptr<Segments> segments1,
							const boost::shared_ptr<LinearObjective> objective1,
							const boost::shared_ptr<Segments> segments2,
							const boost::shared_ptr<LinearObjective> objective2)
{
	bool equal = true;
	boost::unordered_map<boost::shared_ptr<Segment>, unsigned int,
		SegmentPointerHash, SegmentPointerEquals> coreSegmentIdMap;
	unsigned int i = 0;
	std::vector<double> coefs1 = objective1->getCoefficients();
	std::vector<double> coefs2 = objective2->getCoefficients();
	
	foreach (boost::shared_ptr<Segment> segment, segments2->getSegments())
	{
		coreSegmentIdMap[segment] = i++;
	}
	
	i = 0;
	
	foreach (boost::shared_ptr<Segment> segment, segments1->getSegments())
	{
		if (coreSegmentIdMap.count(segment))
		{
			double sval = coefs1[i];
			double cval = coefs2[coreSegmentIdMap[segment]];
			if (cval != sval)
			{
				LOG_DEBUG(segmentstoretestlog) << segment->hashValue() << " " << sval << " " <<
					cval << std::endl;
				equal = false;
			}
		}
		else
		{
			LOG_DEBUG(segmentstoretestlog) << segment->hashValue() << " -1 -1 " << std::endl;
			equal = false;
		}
	}
	
	return equal;
}

bool
SegmentStoreTest::featuresEqual(const boost::shared_ptr<Segments> segments1,
								const boost::shared_ptr<Features> features1,
								const boost::shared_ptr<Segments> segments2,
								const boost::shared_ptr< Features > features2)
{
	bool equal = true;
	boost::unordered_map<boost::shared_ptr<Segment>, unsigned int,
		SegmentPointerHash, SegmentPointerEquals> coreSegmentIdMap;
	
	foreach (boost::shared_ptr<Segment> segment, segments2->getSegments())
	{
		coreSegmentIdMap[segment] = segment->getId();
	}
	
	i = 0;
	
	foreach (boost::shared_ptr<Segment> segment, segments1->getSegments())
	{
		if (coreSegmentIdMap.count(segment))
		{
			std::vector<double>& fv1 = features1.get(segment->getId());
			std::vector<double>& fv2 = features2.get(coreSegmentIdMap[segment]);
			if (fv1 != fv2)
			{
				equal = false;
			}
		}
		else
		{
			equal = false;
		}
	}
	
	return equal;
}

bool
SegmentStoreTest::solutionEqual(const boost::shared_ptr<Segments> segments1,
									 const boost::shared_ptr<Solution> solution1,
									 const boost::shared_ptr<Segments> segments2,
									 const boost::shared_ptr<Solution> solution2)
{
	boost::shared_ptr<Reconstructor> r1 = boost::make_shared<Reconstructor>();
	boost::shared_ptr<Reconstructor> r2 = boost::make_shared<Reconstructor>();
	pipeline::Value<Segments> segmentsR1, segmentsR2;
	
	r1->setInput("solution", solution1);
	r1->setInput("segments", segments1);
	
	r2->setInput("solution", solution2);
	r2->setInput("segments", segments2);
	
	segmentsR1 = r1->getOutput();
	segmentsR2 = r2->getOutput();
	
	return segmentsEqual(segmentsR1, segmentsR2);
}

bool
SegmentStoreTest::segmentsEqual(const boost::shared_ptr<Segments> segments1,
								const boost::shared_ptr<Segments> segments2)
{
	if (segments1->size() != segments2->size())
	{
		return false;
	}
	else
	{
		SegmentSetType segmentSet;
		foreach (boost::shared_ptr<Segment> segment, segments1->getSegments())
		{
			segmentSet.insert(segment);
		}
		
		foreach (boost::shared_ptr<Segment> segment, segments2->getSegments())
		{
			if (!segmentSet.count(segment))
			{
				return false;
			}
		}
		
		return true;
	}
}

bool
SegmentStoreTest::verifyStores(const boost::shared_ptr<SegmentStore> store1,
							   const boost::shared_ptr<SegmentStore> store2,
							   const boost::shared_ptr<BlockManager> blockManager)
{
	boost::shared_ptr<Box<> > box = boost::make_shared<Box<> >(util::point3<unsigned int>(0,0,0),
															   blockManager->stackSize());
	boost::shared_ptr<Blocks> blocks = blockManager->blocksInBox(box);
	boost::shared_ptr<Cores> cores = blockManager->coresInBox(box);
	
	boost::shared_ptr<SegmentReader> localReader = boost::make_shared<SegmentReader>();
	boost::shared_ptr<SegmentReader> testReader = boost::make_shared<SegmentReader>();
	
	boost::shared_ptr<CostReader> localCostReader = boost::make_shared<CostReader>();
	boost::shared_ptr<CostReader> testCostReader = boost::make_shared<CostReader>();	
	boost::shared_ptr<SolutionReader> localSolutionReader = boost::make_shared<SolutionReader>();
	boost::shared_ptr<SolutionReader> testSolutionReader = boost::make_shared<SolutionReader>();
	boost::shared_ptr<SegmentFeatureReader> localFeatureReader =
		boost::make_shared<SegmentFeatureReader>();
	boost::shared_ptr<SegmentFeatureReader> testFeatureReader =
		boost::make_shared<SegmentFeatureReader>();

	boost::shared_ptr<bool> yeah = boost::make_shared<bool>(true);
		
	localReader->setInput("store", store1);
	testReader->setInput("store", store2);
	
	localCostReader->setInput("store", store1);
	testCostReader->setInput("store", store2);
	
	localSolutionReader->setInput("store", store1);
	testSolutionReader->setInput("store", store2);
	
	localFeatureReader->setInput("store", store1);
	testFeatureReader->setInput("store", store2);
	localFeatureReader->setInput("stored only", yeah);
	testFeatureReader->setInput("stored only", yeah);
	
	
	foreach (boost::shared_ptr<Block> block, *blocks)
	{
		boost::shared_ptr<Blocks> singletonBlocks = boost::make_shared<Blocks>();
		singletonBlocks->add(block);
		localReader->setInput("blocks", singletonBlocks);
		testReader->setInput("blocks", singletonBlocks);
		
		pipeline::Value<Segments> localSegments = localReader->getOutput("segments");
		pipeline::Value<Segments> testSegments = testReader->getOutput("segments");
		
		localFeatureReader->setInput("segments", localSegments);
		testFeatureReader->setInput("segments", testSegments);
	}
}


};