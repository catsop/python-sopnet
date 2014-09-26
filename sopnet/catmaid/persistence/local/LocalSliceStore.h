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

class LocalSliceStore : public SliceStore {

	typedef std::map<Slice, boost::shared_ptr<Blocks> > SliceBlockMap;
	typedef std::map<Block, boost::shared_ptr<Slices> > BlockSliceMap;
	typedef std::map<unsigned int, boost::shared_ptr<Slice> > IdSliceMap;
	typedef std::map<unsigned int, boost::shared_ptr<ConflictSets> > IdConflictsMap;

public:

	LocalSliceStore();

	/**
	 * Associate a set of slices to a block.
	 */
	void associateSlicesToBlock(
			const Slices& slices,
			const Block&  block) {}

	/**
	 * Associate a set of conflict sets to a block. The conflict sets are 
	 * assumed to hold the hashes of the slices.
	 */
	void associateConflictSetsToBlock(
			const ConflictSets& conflictSets,
			const Block&        block) {}

	/**
	 * Get all slices that are associated to the given blocks. This creates 
	 * "real" slices in the sense that the geometry of the slices will be 
	 * restored.
	 */
	boost::shared_ptr<Slices> getSlicesByBlock(const Blocks& blocks) {}

	/**
	 * Get all the conflict sets that are associated to the given blocks. The 
	 * conflict sets will contain the hashes of slices.
	 */
	boost::shared_ptr<ConflictSets> getConflictSetsByBlocks(const Blocks& block) {}


	/******************************************
	 * OLD INTERFACE DEFINITION -- DEPRECATED *
	 ******************************************/

    void associate(boost::shared_ptr<Slices> slices, boost::shared_ptr<Block> block);

    boost::shared_ptr<Slices> retrieveSlices(const Blocks& blocks);

	boost::shared_ptr<Blocks> getAssociatedBlocks(boost::shared_ptr<Slice> slice);

	void storeConflict(boost::shared_ptr<ConflictSets> conflictSets);
	
	boost::shared_ptr<ConflictSets> retrieveConflictSets(const Slices& slices);

	void dumpStore();
private:
	
	void mapSliceToBlock(const boost::shared_ptr<Slice> slice,
						 const boost::shared_ptr<Block> block);
	void mapBlockToSlice(const boost::shared_ptr<Block> block,
						 const boost::shared_ptr<Slice> slice);

	boost::shared_ptr<Slice> equivalentSlice(const boost::shared_ptr<Slice> slice);
	boost::shared_ptr<Slices> getSlices(const Block& block);
	boost::shared_ptr<Blocks> getBlocks(const Slice& slice);
	boost::shared_ptr<ConflictSets> getConflictSet(const unsigned int id);
	
	SliceSet _sliceMasterSet;
	SliceBlockMap _sliceBlockMap;
	BlockSliceMap _blockSliceMap;
	IdSliceMap _idSliceMap;
	IdConflictsMap _conflictMap;
	
};

#endif //LOCAL_SLICE_STORE_H__
