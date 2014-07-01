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
     * Retrieve all slices that are at least partially contained in the given blocks as well as
	 * any slices that are in conflict with these.
	 * 
	 * In other words, slice conflict sets may traverse Block boundaries. Because of this, it is
	 * possible for a Slice to be in conflict with another Slice that is not associated with 
	 * the requested Block set. This Slice must also be retrieved in order for Segments to
	 * be guaranteed and for solutions to be calculated.
	 * 
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
	
	/**
	 * Retrieve all ConflictSets for the given Slices
	 * @param slices - the Slices for which ConflictSets are to be returned
	 * @return a ConflictSets object containing each ConflictSet to which any Slice in slices
	 * belongs.
	 */
	virtual pipeline::Value<ConflictSets>
		retrieveConflictSets(pipeline::Value<Slices> slices) = 0;

	/**
	 * Dump the contents of the store to a log channel.
	 */
	virtual void dumpStore() = 0;
};

#endif //SLICE_STORE_H__
