#ifndef TEST_SEGMENT_STORE_H__
#define TEST_SEGMENT_STORE_H__

#include "CatsopTest.h"
#include "BlockManagerTest.h"
#include <catmaid/persistence/SegmentStore.h>
#include <catmaid/persistence/StackStore.h>
#include <catmaid/persistence/SliceStore.h>
#include <boost/shared_ptr.hpp>
#include <sopnet/slices/Slices.h>
#include <sopnet/segments/Segments.h>
#include <sopnet/inference/Solutions.h>

namespace catsoptest
{

class SegmentStoreFactory
{
public:
	virtual boost::shared_ptr<SegmentStore> createSegmentStore() = 0;
};

class SegmentStoreTestParam
{
public:
	SegmentStoreTestParam(){}
	SegmentStoreTestParam(const std::string& inName,
						  const boost::shared_ptr<StackStore> inMembraneStackStore,
						  const boost::shared_ptr<StackStore> inRawStackStore,
						  const boost::shared_ptr<BlockManagerFactory> blockManagerFactory,
						  const boost::shared_ptr<BlockManagerTestParam> blockManagerArg);

	const std::string name;
	const boost::shared_ptr<StackStore> membraneStackStore;
	const boost::shared_ptr<StackStore> rawStackStore;
	boost::shared_ptr<BlockManager> blockManager();
	boost::shared_ptr<BlockManagerTestParam> getBlockManagerParam() const;
	
private:
	boost::shared_ptr<BlockManagerFactory> _factory;
	boost::shared_ptr<BlockManagerTestParam> _blockManagerParam;
};

class SegmentStoreTest : public catsoptest::Test<SegmentStoreTestParam>
{
public:
	SegmentStoreTest(const boost::shared_ptr<SegmentStoreFactory> factory);
	
	bool run(boost::shared_ptr<SegmentStoreTestParam> arg);
	
	std::string name();
	
	std::string reason();
	
	static std::vector<boost::shared_ptr<SegmentStoreTestParam> >
		generateTestParameters(const std::string& name,
							   const util::point3<unsigned int>& stackSize,
							   const boost::shared_ptr<StackStore> stackStore,
							   const boost::shared_ptr<BlockManagerFactory> factory);

private:
	void guaranteeSlices(const boost::shared_ptr<SliceStore> sliceStore,
						 const boost::shared_ptr<StackStore> stackStore,
					  const boost::shared_ptr<BlockManager> blockManager);
	
	void guaranteeSegments(const boost::shared_ptr<SegmentStore> segmentStore,
							const boost::shared_ptr<SliceStore> sliceStore,
							const boost::shared_ptr<StackStore> stackStore,
							const boost::shared_ptr<BlockManager> blockManager);
	
	bool segmentsEqual(const boost::shared_ptr<Segments> segments1,
					   const boost::shared_ptr<Segments> segments2);
	
	void copyStores(const boost::shared_ptr<SegmentStore> store,
					const boost::shared_ptr<SegmentStore> testStore,
					const boost::shared_ptr<BlockManager> blockManager);
	
	bool featuresEqual(const boost::shared_ptr<Segments> segments1,
						const boost::shared_ptr<Features> features1,
						const boost::shared_ptr<Segments> segments2,
						const boost::shared_ptr<Features> features2);
	
	bool costEqual(const boost::shared_ptr<Segments> segments1,
				   const boost::shared_ptr<LinearObjective> objective1,
					const boost::shared_ptr<Segments> segments2,
					const boost::shared_ptr<LinearObjective> objective2);
	
	bool solutionEqual(const boost::shared_ptr<Segments> segments1,
					   const boost::shared_ptr<Solution> solution1,
						const boost::shared_ptr<Segments> segments2,
						const boost::shared_ptr<Solution> solution2);
	
	bool verifyStores(const boost::shared_ptr<SegmentStore> store1,
					  const boost::shared_ptr<SegmentStore> store2,
					  const boost::shared_ptr<BlockManager> blockManager);
	
	
	const boost::shared_ptr<SegmentStoreFactory> _factory;
};

};

#endif //TEST_SEGMENT_STORE_H__