#ifndef SLICE_GUARANTOR_H__
#define SLICE_GUARANTOR_H__

#include <set>
#include <boost/shared_ptr.hpp>

#include <blockwise/persistence/SliceStore.h>
#include <blockwise/persistence/StackStore.h>
#include <blockwise/blocks/Blocks.h>
#include <blockwise/blocks/BlockUtils.h>
#include <slices/ConflictSets.h>
#include <imageprocessing/io/ImageBlockStackReader.h>
#include <imageprocessing/io/ImageBlockFactory.h>
#include <imageprocessing/ComponentTreeExtractorParameters.h>

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
	 * Set non-default parameters for the slice extraction.
	 */
	void setComponentTreeExtractorParameters(const boost::shared_ptr<ComponentTreeExtractorParameters> parameters);

	/**
	 * Extracts the slices for the requested blocks. Returns empty Blocks for 
	 * success. If extraction was not possible, Blocks in the output will 
	 * indicate those for which it failed. Typically, this means that not all 
	 * images were available in the given Blocks.
	 */
	Blocks guaranteeSlices(const Blocks& blocks);

private:

	// Checks whether slices have already been extracted for our Block request.
	bool checkSlices(const Blocks& blocks);

	// extract whole slices in a section for the given requestBlocks
	Blocks extractSlicesAndConflicts(
			Slices&            slices,
			ConflictSets&      conflictSets,
			const Blocks&      requestedBlocks,
			const unsigned int z);

	// associate the extracted slices and conflict sets to the given blocks 
	// using slice store
	void writeSlicesAndConflicts(
			const Slices&       slices,
			const ConflictSets& conflictSets,
			const Blocks&       requestedBlocks,
			const Blocks&       allBlocks);

	// find all slices that have to be extracted completely
	void getRequiredSlicesAndConflicts(
		const Slices&       slices,
		const ConflictSets& conflictSets,
		const Blocks&       requestedBlocks,
		Slices&             requiredSlices,
		ConflictSets&       requiredConflictSets);

	// find a subset of slices that overlap with the given block
	boost::shared_ptr<Slices> collectSlicesByBlock(const Slices& slices, const Block& block);

	// find a subset of conflict sets that involve the given slices
	boost::shared_ptr<ConflictSets> collectConflictsBySlices(const ConflictSets& conflictSets, const Slices& slices);

	// true, if conflictSet contains any slice of slices
	bool containsAny(const ConflictSet& conflictSet, const Slices& slices);
	
	/**
	 * Helper function that checks whether a Slice can be considered whole or
	 * not
	 */
	void checkWhole(const Slice& slice,
					Blocks& nbdBlocks) const;
	
	boost::shared_ptr<ComponentTreeExtractorParameters> _parameters;
	boost::shared_ptr<SliceStore>                       _sliceStore;
	boost::shared_ptr<StackStore>                       _stackStore;

	BlockUtils _blockUtils;
};

#endif //SLICE_GUARANTOR_H__
