#ifndef DJANGO_SLICE_STORE_H__
#define DJANGO_SLICE_STORE_H__

#include <catmaid/persistence/SliceStore.h>
#include <sopnet/slices/Slices.h>
#include <catmaid/blocks/Blocks.h>
#include <sopnet/slices/ConflictSets.h>

/**
 * A SliceStore backed by a JSON interface via django, ie, CATMAID-style. This SliceStore
 * requires/assumes that the Blocks used to store and retrieve data are consistent with those
 * already in the database, ie, as returned by a DjangoBlockManager.
 */
class DjangoSliceStore : public SliceStore {

public:

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
	boost::shared_ptr<Slices> getSlicesByBlocks(
			const Blocks& blocks,
			Blocks&       missingBlocks) {}

	/**
	 * Get all the conflict sets that are associated to the given blocks. The 
	 * conflict sets will contain the hashes of slices.
	 */
	boost::shared_ptr<ConflictSets> getConflictSetsByBlocks(
			const Blocks& block,
			Blocks&       missingBlocks) {}
};

#endif //DJANGO_SLICE_STORE_H__
