#ifndef SLICE_GUARANTOR_H__
#define SLICE_GUARANTOR_H__

#include <set>
#include <boost/shared_ptr.hpp>

#include <catmaid/persistence/SliceStore.h>
#include <catmaid/persistence/StackStore.h>
#include <catmaid/blocks/Blocks.h>
#include <catmaid/blocks/BlockUtils.h>
#include <sopnet/slices/ConflictSets.h>
#include <imageprocessing/io/ImageBlockStackReader.h>
#include <imageprocessing/io/ImageBlockFactory.h>
#include <imageprocessing/MserParameters.h>

/**
 * SliceGuarantor is a class that, given a sub-stack (measured in Blocks), guarantees that all
 * Slices in that substack will exist in the given SliceStore after guaranteeSlices is called.
 * If this guarantee cannot be made, say because the membrane images are not yet available,
 * guaranteeSlices will report those Blocks and exit (in this case, no guarantee about the slices
 * has been made).
 *
 * This extends to any conflict sets over the slices. In the MSER sense, two Slices conflict if
 * one is the ancestor of the other. A SliceGuarantor will guarantee that all Slices that belong to
 * the same conflict set as a Slice in the guaranteed substack are also populated in the Slice Store,
 * even if they don't overlap with the guaranteed substack.
 */
class SliceGuarantor {

public:

	/**
	 * Create a new SliceGuarantor using the given slice- and stack-store as 
	 * backends.
	 */
	SliceGuarantor(
			const ProjectConfiguration&   projectConfiguration,
			boost::shared_ptr<SliceStore> sliceStore,
			boost::shared_ptr<StackStore> stackStore);

	/**
	 * Set non-default mser parameters for the slice extraction.
	 */
	void setMserParameters(const boost::shared_ptr<MserParameters> mserParameters);

	/**
	 * Extracts the slices for the requested blocks. Returns empty Blocks for 
	 * success. If extraction was not possible, Blocks in the output will 
	 * indicate those for which it failed. Typically, this means that not all 
	 * images were available in the given Blocks.
	 */
	Blocks guaranteeSlices(const Blocks& blocks);

private:

	bool checkSlices(const Blocks& blocks);
	
	void collectOutputSlices(
			const Slices& extractedSlices,
			const ConflictSets& extractedConflict,
			Slices& slices,
			const Blocks& blocks);
	
	// extract whole slices in a section for the given requestBlocks
	void extractSlices(
			const unsigned int z,
			Slices& slices,
			ConflictSets& conflictSets,
			Blocks& extractBlocks,
			const Blocks& requestBlocks);

	// associate the extracted slices and conflict sets to the given blocks 
	// using slice store
	void writeSlicesAndConflicts(
			const Slices&       slices,
			const ConflictSets& conflictSets,
			const Blocks&       blocks);

	// find a subset of slices that overlap with the given block
	boost::shared_ptr<Slices> collectSlicesByBlock(const Slices& slices, const Block& block);

	// find a subset of conflict sets that involve the given slices
	boost::shared_ptr<ConflictSets> collectConflictBySlices(const ConflictSets& conflictSets, const Slices& slices);

	// true, if conflictSet contains any slice of slices
	bool containsAny(const ConflictSet& conflictSet, const Slices& slices);

	bool containsAny(const ConflictSet& conflictSet, const std::set<unsigned int>& idSet);
	
	/**
	 * Helper function that checks whether a Slice can be considered whole or
	 * not
	 */
	void checkWhole(const Slice& slice,
					const Blocks& extractBlocks,
					Blocks& nbdBlocks) const;
	
	boost::shared_ptr<MserParameters> _mserParameters;
	boost::shared_ptr<SliceStore>     _sliceStore;
	boost::shared_ptr<StackStore>     _stackStore;

	BlockUtils _blockUtils;
};

#endif //SLICE_GUARANTOR_H__
