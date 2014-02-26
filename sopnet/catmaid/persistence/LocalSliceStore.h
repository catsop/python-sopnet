#ifndef LOCAL_SLICE_STORE_H__
#define LOCAL_SLICE_STORE_H__

#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/shared_ptr.hpp>

#include <catmaid/persistence/SlicePointerHash.h>
#include <catmaid/persistence/SliceStore.h>

/**
 * A SliceStore implemented locally in RAM for testing purposes.
 */

class LocalSliceStore : public SliceStore
{
	typedef std::map<Slice, pipeline::Value<Blocks> > SliceBlockMap;
	typedef std::map<Block, pipeline::Value<Slices> > BlockSliceMap;
	typedef std::map<unsigned int, boost::shared_ptr<Slice> > IdSliceMap;
	typedef std::map<unsigned int, pipeline::Value<ConflictSets> > IdConflictsMap;

public:
	LocalSliceStore();

    void associate(pipeline::Value<Slices> slices, pipeline::Value<Block> block);

    pipeline::Value<Slices> retrieveSlices(pipeline::Value<Blocks> blocks);

	pipeline::Value<Blocks> getAssociatedBlocks(pipeline::Value<Slice> slice);

	void storeConflict(pipeline::Value<ConflictSets> conflictSets);
	
	pipeline::Value<ConflictSets> retrieveConflictSets(pipeline::Value<Slices> slices);

	void dumpStore();
private:
	
	void mapSliceToBlock(const boost::shared_ptr<Slice> slice,
						 const boost::shared_ptr<Block> block);
	void mapBlockToSlice(const boost::shared_ptr<Block> block,
						 const boost::shared_ptr<Slice> slice);

	boost::shared_ptr<Slice> equivalentSlice(const boost::shared_ptr<Slice> slice);
	
	SliceSet _sliceMasterSet;
	SliceBlockMap _sliceBlockMap;
	BlockSliceMap _blockSliceMap;
	IdSliceMap _idSliceMap;
	IdConflictsMap _conflictMap;
	
};

#endif //LOCAL_SLICE_STORE_H__
