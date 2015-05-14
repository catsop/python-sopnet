#ifndef LOCAL_SLICE_STORE_H__
#define LOCAL_SLICE_STORE_H__

#include <boost/shared_ptr.hpp>
#include <blockwise/persistence/SliceStore.h>

/**
 * A SliceStore implemented locally in RAM for testing purposes.
 */

class LocalSliceStore : public SliceStore {

public:

	LocalSliceStore() {}

	/**
	 * Associate a set of slices to a block.
	 */
	void associateSlicesToBlock(
			const Slices& slices,
			const Block&  block,
			bool  doneWithBlock = true);

	/**
	 * Associate a set of conflict sets to a block. The conflict sets are 
	 * assumed to hold the hashes of the slices.
	 */
	void associateConflictSetsToBlock(
			const ConflictSets& conflictSets,
			const Block&        block);

	/**
	 * Get all slices that are associated to the given blocks. This creates 
	 * "real" slices in the sense that the geometry of the slices will be 
	 * restored.
	 */
	boost::shared_ptr<Slices> getSlicesByBlocks(
			const Blocks& blocks,
			Blocks&       missingBlocks);

	/**
	 * Get all slices that are used by the segments with the given hashes. This 
	 * method does not check whether slices for all involved blocks have been 
	 * extracted, yet.
	 *
	 * @param segmentHashes
	 *              A set of segment hashes, for which to get the slices.
	 */
	boost::shared_ptr<Slices> getSlicesBySegmentHashes(
			const std::set<SegmentHash>& /*segmentHashes*/) {

		UTIL_THROW_EXCEPTION(NotYetImplemented, "");
	}

	/**
	 * Get all the conflict sets that are associated to the given blocks. The 
	 * conflict sets will contain the hashes of slices.
	 */
	boost::shared_ptr<ConflictSets> getConflictSetsByBlocks(
			const Blocks& blocks,
			Blocks&       missingBlocks);

	/**
	 * Check whether the slices for the given block have already been extracted.
	 */
	bool getSlicesFlag(const Block& block);

private:

	std::map<Block, Slices> _slices;

	std::map<Block, ConflictSets> _conflictSets;

	std::set<Block> _slicesFlags;
};

#endif //LOCAL_SLICE_STORE_H__
