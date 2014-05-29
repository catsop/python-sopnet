#ifndef TEST_SLICE_STORE_H__
#define TEST_SLICE_STORE_H__

#include "CatsopTest.h"
#include "BlockManagerTest.h"
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
						const boost::shared_ptr<BlockManagerFactory> blockManagerFactory,
						const boost::shared_ptr<BlockManagerTestParam> blockManagerArg);

	const std::string name;
	const boost::shared_ptr<StackStore> stackStore;
	boost::shared_ptr<BlockManager> blockManager();
	boost::shared_ptr<BlockManagerTestParam> getBlockManagerParam() const;

private:
	boost::shared_ptr<BlockManagerFactory> _factory;
	boost::shared_ptr<BlockManagerTestParam> _blockManagerParam;
};


class SliceStoreTest : public catsoptest::Test<SliceStoreTestParam>
{
public:
	SliceStoreTest(const boost::shared_ptr<SliceStoreFactory> factory);
	
	bool run(boost::shared_ptr<SliceStoreTestParam> arg);
	
	std::string name();
	
	std::string reason();
	
	static std::vector<boost::shared_ptr<SliceStoreTestParam> >
		generateTestParameters(const std::string& name,
							   const util::point3<unsigned int>& stackSize,
							   const boost::shared_ptr<StackStore> stackStore,
							   const boost::shared_ptr<BlockManagerFactory> factory);
		
private:
	void guaranteeSlices(const boost::shared_ptr<SliceStore> sliceStore,
						 const boost::shared_ptr<StackStore> stackStore,
						 const boost::shared_ptr<BlockManager> blockManager);
	
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

std::ostream& operator<<(std::ostream& os, const catsoptest::SliceStoreTestParam& param);

#endif //TEST_SLICE_STORE_H__