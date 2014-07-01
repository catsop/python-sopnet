#ifndef SEGMENT_GUARANTOR_H__
#define SEGMENT_GUARANTOR_H__

#include <catmaid/SliceGuarantor.h>
#include <catmaid/persistence/SegmentStore.h>
#include <sopnet/block/BlockManager.h>
#include <catmaid/persistence/SliceReader.h>
#include <catmaid/persistence/SegmentWriter.h>
#include <catmaid/persistence/StackStore.h>
#include <sopnet/features/Features.h>

class SegmentGuarantor
{

public:

	/**
	 * Set the segment store to be used to write the found segments.
	 */
	void setSegmentStore(boost::shared_ptr<SegmentStore> segmentStore);

	/**
	 * Set the slice store to be used to read slices from which to extract 
	 * segments.
	 */
	void setSliceStore(boost::shared_ptr<SliceStore> sliceStore);

	/**
	 * Set the image stack store to be used to extract segment features.
	 */
	void setRawStackStore(boost::shared_ptr<StackStore> rawStackStore);

	/**
	 * Guarantee segments in the given blocks. If the request can not be 
	 * processed due to missing slices, the returned blocks are non-emtpy, 
	 * containing the blocks for which slices are still missing.
	 */
	Blocks guaranteeSegments(const Blocks& requestedBlocks);

private:

	// get a subset of slices for a given section
	boost::shared_ptr<Slices> collectSlicesByZ(
			Slices& slices,
			unsigned int z) const;

	// compute the bounding box of a set of slices
	Box<> slicesBoundingBox(const Slices& slices);

	bool checkBlockSlices(
			const Blocks& sliceBlocks,
			Blocks&       needBlocks);

	// extract the features for the given segments
	boost::shared_ptr<Features> guaranteeFeatures(
			const boost::shared_ptr<Segments> segments);

	// write the segments using the provided segment store
	void writeSegments(const Segments& segments, const Blocks& blocks);

	// test, whether a segment should be associated to a block
	bool associated(
		const Segment& segment,
		const Block& block);

	// write the features of the segments using the provided segment store
	void writeFeatures(const Features& features);

	boost::shared_ptr<SegmentStore> _segmentStore;
	boost::shared_ptr<SliceStore>   _sliceStore;
	boost::shared_ptr<StackStore>   _rawStackStore;
	boost::shared_ptr<BlockManager> _blockManager;
};

#endif //SEGMENT_GUARANTOR_H__
