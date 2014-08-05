#ifndef SLICE_STORE_H__
#define SLICE_STORE_H__

#include <boost/shared_ptr.hpp>

#include <sopnet/slices/Slice.h>
#include <sopnet/slices/ConflictSets.h>

#include <sopnet/slices/Slices.h>
#include <sopnet/block/Blocks.h>

#include <pipeline/Value.h>
#include <pipeline/Data.h>

#include <util/exceptions.h>

struct SliceCacheError : virtual Exception {};

/**
 * Abstract Data class that handles the practicalities of storing and retrieving Slices from a
 * store.
 */
class SliceStore : public pipeline::Data
{
public:
	
	/**
	 * Write Slices and ConflictSets to the store.
	 * conflictSets is considered optional. Leave empty in order not to write.
	 */
	void writeSlices(const Slices& slices, const ConflictSets& conflictSets, const Blocks& blocks);
	
    /**
     * Associates a slice with a block
     * @param slices - the slices to store.
     * @param block - the block containing the slices.
     */
    virtual void associate(boost::shared_ptr<Slices> slices,
						   boost::shared_ptr<Block> block) = 0;

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
    virtual boost::shared_ptr<Slices> retrieveSlices(const Blocks& blocks) = 0;

	/**
	 * Retrieve all Blocks associated with the given slice.
	 * @param slice - the slice for which Blocks are to be retrieved.
	 */
	virtual boost::shared_ptr<Blocks> getAssociatedBlocks(
		boost::shared_ptr<Slice> slice) = 0;
	
	/**
	 * Store a conflict set relationship. This requires the slices in question to already be
	 * stored in this SliceStore. If this is not the case, this function will throw a 
	 * SliceCacheError
	 * @param conflictSets - the ConflictSets in question
	 */
	virtual void storeConflict(boost::shared_ptr<ConflictSets> conflictSets) = 0;
	
	/**
	 * Retrieve all ConflictSets for the given Slices
	 * @param slices - the Slices for which ConflictSets are to be returned
	 * @return a ConflictSets object containing each ConflictSet to which any Slice in slices
	 * belongs.
	 */
	virtual boost::shared_ptr<ConflictSets>
		retrieveConflictSets(const Slices& slices) = 0;

	/**
	 * Dump the contents of the store to a log channel.
	 */
	virtual void dumpStore() = 0;
	
private:
	bool containsAny(ConflictSet& conflictSet, const boost::shared_ptr<Slices> slices);
	
	boost::shared_ptr<Slices> collectSlicesByBlocks(const Slices& slices,
													const boost::shared_ptr<Block> block);
	boost::shared_ptr<ConflictSets> collectConflictBySlices(const boost::shared_ptr<Slices> slices,
															const ConflictSets& conflictSets);
};

#endif //SLICE_STORE_H__
