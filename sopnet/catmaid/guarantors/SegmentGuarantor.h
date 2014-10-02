#ifndef SEGMENT_GUARANTOR_H__
#define SEGMENT_GUARANTOR_H__

#include <catmaid/ProjectConfiguration.h>
#include <catmaid/blocks/BlockUtils.h>
#include <catmaid/persistence/SegmentStore.h>
#include <catmaid/persistence/SliceStore.h>
#include <catmaid/persistence/StackStore.h>
#include <sopnet/segments/Segments.h>
#include <sopnet/features/Features.h>

class SegmentGuarantor {

public:

	SegmentGuarantor(
			const ProjectConfiguration&     projectConfiguration,
			boost::shared_ptr<SegmentStore> segmentStore,
			boost::shared_ptr<SliceStore>   sliceStore,
			boost::shared_ptr<StackStore>   rawStackStore);

	/**
	 * Guarantee segments in the given blocks. If the request can not be 
	 * processed due to missing slices, the returned blocks are non-emtpy, 
	 * containing the blocks for which slices are still missing.
	 */
	Blocks guaranteeSegments(const Blocks& requestedBlocks);

private:

	// check whether segments are already present for the given blocks
	bool alreadyPresent(const Blocks& blocks);

	// get all slices for sliceBlocks -- blocks with missing slices will be 
	// reported in missingBlocks
	boost::shared_ptr<Slices> getSlices(Blocks sliceBlocks, Blocks& missingBlocks);

	// get all segment descriptions for segments that overlap with the given 
	// block
	SegmentDescriptions getSegmentDescriptions(
			const Segments& segments,
			const Features& features,
			const Block&    block);

	// find all segments that are overlapping with the requested blocks
	boost::shared_ptr<Segments>
	discardNonRequestedSegments(
			boost::shared_ptr<Segments> allSegments,
			const Blocks&               requestedBlocks);

	// use the provided segment store to save the extracted segments and 
	// features for the requested blocks
	void writeSegmentsAndFeatures(
			const Segments& segments,
			const Features& features,
			const Blocks&   requestedBlocks);

	// check if a segment overlaps with a block
	bool overlaps(const Segment& segment, const Block& block);

	// get a subset of slices for a given section
	boost::shared_ptr<Slices> collectSlicesByZ(
			Slices& slices,
			unsigned int z) const;

	// compute the bounding box of a set of slices
	util::box<unsigned int> slicesBoundingBox(const Slices& slices);

	// extract the features for the given segments
	boost::shared_ptr<Features> computeFeatures(const boost::shared_ptr<Segments> segments);

	boost::shared_ptr<SegmentStore> _segmentStore;
	boost::shared_ptr<SliceStore>   _sliceStore;
	boost::shared_ptr<StackStore>   _rawStackStore;

	BlockUtils _blockUtils;
};

#endif //SEGMENT_GUARANTOR_H__
