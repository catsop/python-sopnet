#ifndef TEST_SLICE_STORE_H__
#define TEST_SLICE_STORE_H__

#include "CatsopTest.h"
#include <catmaid/persistence/SliceStore.h>
#include <catmaid/persistence/StackStore.h>
#include <map>

namespace catsoptest
{

class SliceStoreFactory
{
public:
	virtual boost::shared_ptr<SliceStore> createSliceStore() = 0;
};

class SliceStoreTestParam
{
public:
	SliceStoreTestParam() {};
	SliceStoreTestParam(const std::string& inName,
						const boost::shared_ptr<StackStore> inStackStore,
						const boost::shared_ptr<BlockManager> inBlockManager) :
						name(inName),
						stackStore(inStackStore),
						blockManager(inBlockManager) {}

	const std::string name;
	const boost::shared_ptr<StackStore> stackStore;
	const boost::shared_ptr<BlockManager> blockManager;
};

	
class SliceStoreTest : public catsoptest::Test<SliceStoreTestParam>
{
public:
	SliceStoreTest(const boost::shared_ptr<SliceStoreFactory> factory);
	
	bool run(boost::shared_ptr<SliceStoreTestParam> arg);
	
	std::string name();
	
	std::string reason();
private:
	void guaranteeSlices(const boost::shared_ptr<SliceStore> store,
						 const boost::shared_ptr<SliceStoreTestParam> arg);
	
	void copyStores(const boost::shared_ptr<SliceStore> store,
					const boost::shared_ptr<SliceStore> testStore,
					const boost::shared_ptr<BlockManager> blockManager);
	
	bool conflictSetsEqual(const boost::shared_ptr<Slices> slices1,
						  const boost::shared_ptr<ConflictSets> sets1,
						  const boost::shared_ptr<Slices> slices2,
						  const boost::shared_ptr<ConflictSets> sets2);
	
	bool slicesEqual(const boost::shared_ptr<Slices> slices1,
					 const boost::shared_ptr<Slices> slices2);
	
	bool verifyStores(const boost::shared_ptr<SliceStore> store,
					  const boost::shared_ptr<SliceStore> testStore,
					  const boost::shared_ptr<BlockManager> blockManager);
	
	bool conflictContains(boost::shared_ptr<ConflictSets> sets,
						  const ConflictSet& set);
	
	boost::shared_ptr<Blocks> singletonBlocks(const boost::shared_ptr<Block> block);
	
	boost::shared_ptr<ConflictSets> mapConflictSets(
		const boost::shared_ptr<ConflictSets> conflictSets,
		std::map<unsigned int, unsigned int>& idMap);
	
	boost::shared_ptr<SliceStoreFactory> _factory;
	std::stringstream _reason;
};
	
};


#endif //TEST_SLICE_STORE_H__