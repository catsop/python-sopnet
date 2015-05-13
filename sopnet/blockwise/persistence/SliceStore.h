#ifndef SLICE_STORE_H__
#define SLICE_STORE_H__

#include <boost/shared_ptr.hpp>

#include <slices/Slice.h>
#include <slices/Slices.h>
#include <slices/ConflictSets.h>
#include <blockwise/blocks/Blocks.h>

#include <util/exceptions.h>

/**
 * Slice store interface definition.
 */
class SliceStore {

public:

	/**
	 * Associate a set of slices to a block.
	 */
	virtual void associateSlicesToBlock(
			const Slices& slices,
			const Block&  block,
			bool  doneWithBlock = true) = 0;

	/**
	 * Associate a set of conflict sets to a block. The conflict sets are 
	 * assumed to hold the hashes of the slices.
	 */
	virtual void associateConflictSetsToBlock(
			const ConflictSets& conflictSets,
			const Block&        block) = 0;

	/**
	 * Get all slices that are associated to the given blocks. This creates 
	 * "real" slices in the sense that the geometry of the slices will be 
	 * restored.
	 *
	 * @param blocks
	 *              The requested blocks to get the slices for.
	 *
	 * @param missingBlocks
	 *              A reference to a block collection. This collection will be 
	 *              filled with the blocks for which no slices are available, 
	 *              yet. This collection will be empty on success.
	 */
	virtual boost::shared_ptr<Slices> getSlicesByBlocks(
			const Blocks& blocks,
			Blocks&       missingBlocks) = 0;

	/**
	 * Get all the conflict sets that are associated to the given blocks. The 
	 * conflict sets will contain the hashes of slices.
	 *
	 * @param blocks
	 *              The requested blocks to get the conflict sets for.
	 *
	 * @param missingBlocks
	 *              A reference to a block collection. This collection will be 
	 *              filled with the blocks for which no conflict sets are 
	 *              available, yet. This collection will be empty on success.
	 */
	virtual boost::shared_ptr<ConflictSets> getConflictSetsByBlocks(const Blocks& block, Blocks& missingBlocks) = 0;

	/**
	 * Check whether the slices for the given block have already been extracted.
	 */
	virtual bool getSlicesFlag(const Block& block) = 0;
};

#endif //SLICE_STORE_H__
