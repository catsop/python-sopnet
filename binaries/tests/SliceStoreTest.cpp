#include "SliceStoreTest.h"
#include <catmaid/SliceGuarantor.h>
#include <catmaid/persistence/local/LocalSliceStore.h>
#include <catmaid/persistence/SlicePointerHash.h>

logger::LogChannel slicestoretestlog("slicestoretestlog", "[SliceStoreTest] ");

namespace catsoptest
{

SliceStoreTestParam::SliceStoreTestParam(const std::string& inName,
								const boost::shared_ptr<StackStore> inStackStore,
								const boost::shared_ptr<BlockManagerFactory> blockManagerFactory,
								const boost::shared_ptr<BlockManagerTestParam> blockManagerArg) :
	name(inName), stackStore(inStackStore),
	_factory(blockManagerFactory), _blockManagerParam(blockManagerArg)
{
	
}

boost::shared_ptr<BlockManager>
SliceStoreTestParam::blockManager()
{
	return _factory->createBlockManager(_blockManagerParam->blockSize,
								 _blockManagerParam->coreSizeInBlocks);
}

boost::shared_ptr<BlockManagerTestParam>
SliceStoreTestParam::getBlockManagerParam() const
{
	return _blockManagerParam;
}


SliceStoreTest::SliceStoreTest(const boost::shared_ptr<SliceStoreFactory> factory) :
	_factory(factory)
{

}

bool
SliceStoreTest::run(boost::shared_ptr<SliceStoreTestParam> arg)
{
	// Use a local slice store for temporary storage. We'll push/pull into the test store
	// after extraction.
	boost::shared_ptr<BlockManager> blockManager = arg->blockManager();
	boost::shared_ptr<SliceStore> store = boost::make_shared<LocalSliceStore>();
	boost::shared_ptr<SliceStore> testStore = _factory->createSliceStore();
	
	
	guaranteeSlices(store, arg->stackStore, blockManager);
	copyStores(store, testStore, blockManager);
	return verifyStores(store, testStore, blockManager);
}



std::string
SliceStoreTest::name()
{
	return "SliceStore test";
}

std::string
SliceStoreTest::reason()
{
	return _reason.str();
}

std::vector<boost::shared_ptr<SliceStoreTestParam> >
SliceStoreTest::generateTestParameters(const std::string& name,
									   const util::point3< unsigned int >& stackSize,
									   const boost::shared_ptr<StackStore> stackStore,
									   const boost::shared_ptr<BlockManagerFactory> factory)
{
	std::vector<boost::shared_ptr<BlockManagerTestParam> > blockParams = 
		BlockManagerTest::generateTestParameters(stackSize);
	std::vector<boost::shared_ptr<SliceStoreTestParam> > sliceStoreParams;

	foreach (boost::shared_ptr<BlockManagerTestParam> bParam, blockParams)
	{
		boost::shared_ptr<SliceStoreTestParam> sliceStoreParam =
			boost::make_shared<SliceStoreTestParam>(name, stackStore, factory, bParam);
		sliceStoreParams.push_back(sliceStoreParam);
	}
	
	return sliceStoreParams;
}


bool
SliceStoreTest::verifyStores(const boost::shared_ptr<SliceStore> store,
							 const boost::shared_ptr<SliceStore> testStore,
							 const boost::shared_ptr<BlockManager> blockManager)
{
	Box<> box(
			util::point3<unsigned int>(0,0,0),
			blockManager->stackSize());
	boost::shared_ptr<Blocks> blocks = blockManager->blocksInBox(box);
	
	foreach (boost::shared_ptr<Block> block, *blocks)
	{
		boost::shared_ptr<Blocks> singleton = singletonBlocks(block);
		boost::shared_ptr<Slices> localSlices = store->retrieveSlices(*singleton);
		boost::shared_ptr<Slices> testSlices = testStore->retrieveSlices(*singleton);
		boost::shared_ptr<ConflictSets> localSets = store->retrieveConflictSets(*localSlices);
		boost::shared_ptr<ConflictSets> testSets = store->retrieveConflictSets(*testSlices);
		
		localSlices->size();
		testSlices->size();
		
		if (!slicesEqual(localSlices, testSlices))
		{
			_reason << "Slices for block " << *block << " were unequal" << std::endl;
			LOG_DEBUG(slicestoretestlog) << "Unequal Slices objects" << std::endl;
			LOG_DEBUG(slicestoretestlog) << "\tFor local, read " << localSlices->size()
				<< " slices" << std::endl;
			LOG_DEBUG(slicestoretestlog) << "\tFor test, read " << testSlices->size() << std::endl;
			return false;
		}
		else
		{
			LOG_DEBUG(slicestoretestlog) << "Retrieved equal slices" << std::endl;
		}
		
		if(!conflictSetsEqual(localSlices, localSets, testSlices, testSets))
		{
			_reason << "ConflictSets for block " << *block << " were unequal" << std::endl;
			LOG_DEBUG(slicestoretestlog) << "Unequals ConflictSets objects" << std::endl;
			LOG_DEBUG(slicestoretestlog) << "\tFor local, read " << localSets->size() <<
				" ConflictSets" << std::endl;
			LOG_DEBUG(slicestoretestlog) << "\tFor test, read " << testSets->size() << std::endl;
			
			return false;
		}
	}
	
	return true;
}


void
SliceStoreTest::copyStores(const boost::shared_ptr<SliceStore> store,
						   const boost::shared_ptr<SliceStore> testStore,
						   const boost::shared_ptr<BlockManager> blockManager)
{
	Box<> box(
			util::point3<unsigned int>(0,0,0),
			blockManager->stackSize());
	boost::shared_ptr<Blocks> blocks = blockManager->blocksInBox(box);
	
	boost::shared_ptr<Slices> slices = store->retrieveSlices(*blocks);
	boost::shared_ptr<ConflictSets> conflictSets = store->retrieveConflictSets(*slices);
	
	testStore->writeSlices(*slices, *conflictSets, *blocks);
}


void
SliceStoreTest::guaranteeSlices(const boost::shared_ptr<SliceStore> sliceStore,
						 const boost::shared_ptr<StackStore> stackStore,
						 const boost::shared_ptr<BlockManager> blockManager)
{
	boost::shared_ptr<SliceGuarantor> guarantor = boost::make_shared<SliceGuarantor>();
	Box<> box(
			util::point3<unsigned int>(0,0,0),
			blockManager->stackSize());
	boost::shared_ptr<Blocks> blocks = blockManager->blocksInBox(box);
	
	guarantor->setInput("blocks", blocks);
	guarantor->setInput("slice store", sliceStore);
	guarantor->setInput("stack store", stackStore);
	
	guarantor->guaranteeSlices();
}

bool SliceStoreTest::slicesEqual(const boost::shared_ptr<Slices> slices1,
								 const boost::shared_ptr<Slices> slices2)
{
	SliceSet sliceSet;
	foreach (boost::shared_ptr<Slice> slice, *slices1)
	{
		sliceSet.insert(slice);
	}
	
	foreach (boost::shared_ptr<Slice> slice, *slices2)
	{
		if (!sliceSet.count(slice))
		{
			return false;
		}
	}
	
	return true;
}

boost::shared_ptr<Blocks>
SliceStoreTest::singletonBlocks(const boost::shared_ptr<Block> block)
{
	boost::shared_ptr<Blocks> blocks = boost::make_shared<Blocks>();
	blocks->add(block);
	return blocks;
}


bool
SliceStoreTest::conflictSetsEqual(const boost::shared_ptr<Slices> slices1,
								  const boost::shared_ptr<ConflictSets> sets1,
								  const boost::shared_ptr<Slices> slices2,
								  const boost::shared_ptr<ConflictSets> sets2)
{
	SliceSet sliceSet;
	std::map<unsigned int, unsigned int> idMap;
	boost::shared_ptr<ConflictSets> mappedSets2;
	
	foreach (boost::shared_ptr<Slice> slice, *slices1)
	{
		sliceSet.insert(slice);
	}
	
	foreach (boost::shared_ptr<Slice> slice2, *slices2)
	{
		boost::shared_ptr<Slice> slice1 = *(sliceSet.find(slice2));
		idMap[slice2->getId()] = slice1->getId();
	}
	
	mappedSets2 = mapConflictSets(sets2, idMap);
	
	// AIEEEEE O(n^2)
	foreach (ConflictSet& set1, *sets1)
	{
		if (!conflictContains(mappedSets2, set1))
		{
			LOG_ERROR(slicestoretestlog) << "Set 1" << std::endl;
			logConflictSets(sets1, slices1);
			LOG_ERROR(slicestoretestlog) << "Set 2" << std::endl;
			logConflictSets(sets2, slices2);
			LOG_ERROR(slicestoretestlog) << "Set 2, mapped to Set 1" << std::endl;
			logConflictSets(mappedSets2, slices1);
			return false;
		}
	}
	
	return true;
}

void
SliceStoreTest::logConflictSets(const boost::shared_ptr<ConflictSets> sets,
								const boost::shared_ptr<Slices> slices)
{
	std::map<unsigned int, boost::shared_ptr<Slice> > idMap;
	foreach (boost::shared_ptr<Slice> slice, *slices)
	{
		idMap[slice->getId()] = slice;
	}
	
	foreach (const ConflictSet& conflict, *sets)
	{
		std::stringstream ss;
		ss << conflict << ": ";
		foreach (unsigned int id, conflict.getSlices())
		{
			ss << idMap[id]->hashValue() << " ";
		}

		LOG_DEBUG(slicestoretestlog) << ss.str() << std::endl;
	}
}


bool
SliceStoreTest::conflictContains(boost::shared_ptr<ConflictSets> sets, const ConflictSet& set)
{
	foreach (ConflictSet& other, *sets)
	{
		if (other == set)
		{
			return true;
		}
	}
	
	return false;
}

boost::shared_ptr<ConflictSets>
SliceStoreTest::mapConflictSets(const boost::shared_ptr<ConflictSets> conflictSets,
								 std::map<unsigned int, unsigned int>& idMap)
{
	boost::shared_ptr<ConflictSets> mappedSets = boost::make_shared<ConflictSets>();
	foreach (ConflictSet conflictSet, *conflictSets)
	{
		ConflictSet mappedSet;
		
		foreach (unsigned int id, conflictSet.getSlices())
		{
			mappedSet.addSlice(idMap[id]);
		}
		
		mappedSets->add(mappedSet);
	}
	
	return mappedSets;
}

};

std::ostream& operator<<(std::ostream& os, const catsoptest::SliceStoreTestParam& param)
{
	os << param.name << ": " << *(param.getBlockManagerParam());
	return os;
}
