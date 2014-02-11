#ifndef SLICE_STORE_H__
#define SLICE_STORE_H__

#include <boost/shared_ptr.hpp>

#include <sopnet/slices/Slice.h>
#include <sopnet/slices/ConflictSets.h>

#include <sopnet/slices/Slices.h>
#include <sopnet/block/Blocks.h>

#include <pipeline/Value.h>
#include <pipeline/Data.h>

/**
 * Abstract Data class that handles the practicalities of storing and retrieving Slices from a
 * store.
 */
class SliceStore : public pipeline::Data
{
public:
    /**
     * Associates a slice with a block
     * @param slices - the slices to store.
     * @param block - the block containing the slices.
     */
    virtual void associate(pipeline::Value<Slices> slices,
						   pipeline::Value<Block> block) = 0;

    /**
     * Retrieve all slices that are at least partially contained in the given blocks.
     * @param blocks - the Blocks for which to retrieve all slices.
     */
    virtual pipeline::Value<Slices> retrieveSlices(pipeline::Value<Blocks> blocks) = 0;

	/**
	 * Retrieve all Blocks associated with the given slice.
	 * @param slice - the slice for which Blocks are to be retrieved.
	 */
	virtual pipeline::Value<Blocks> getAssociatedBlocks(
		pipeline::Value<Slice> slice) = 0;
	
	/**
	 * Store a conflict set relationship. This requires the slices in question to already be
	 * stored in this SliceStore.
	 * @param conflictSets - the ConflictSets in question
	 */
	virtual void storeConflict(pipeline::Value<ConflictSets> conflictSets) = 0;
	
	virtual pipeline::Value<ConflictSets>
		retrieveConflictSets(pipeline::Value<Slices> slices) = 0;
};

#endif //SLICE_STORE_H__