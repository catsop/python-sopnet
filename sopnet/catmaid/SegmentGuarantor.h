#ifndef SEGMENT_GUARANTOR_H__
#define SEGMENT_GUARANTOR_H__

#include <pipeline/all.h>
#include <catmaid/SliceGuarantor.h>
#include <catmaid/persistence/SegmentStore.h>
#include <sopnet/block/BlockManager.h>
#include <catmaid/persistence/SliceReader.h>
#include <catmaid/persistence/SegmentWriter.h>
#include <catmaid/persistence/StackStore.h>
#include <sopnet/features/Features.h>

class SegmentGuarantor : public pipeline::SimpleProcessNode<>
{
public:
	/**
	 * Construct a SegmentGuarantor
	 * Inputs:
	 *   Blocks "blocks" - requested blocks
	 *   SegmentStore "segment store"
	 *   SliceStore "slice store"
	 *   StackStore "stack store" - raw images, optional
	 * 
	 * If "stack store" is set, this SegmentGuarantor will extract Features from the guaranteed
	 * Segments, and store them in the SegmentStore.
	 *  
	 * Outputs:
	 *   Blocks "slices blocks" - blocks for which Slices must be guaranteed as a prerequisite
	 *                            to guaranteeing Segments
	 */
	SegmentGuarantor();
	
	
	pipeline::Value<Blocks> guaranteeSegments();
	
private:
	void updateOutputs();

	boost::shared_ptr<Slices> collectSlicesByZ(const boost::shared_ptr<Slices> slices,
											   unsigned int z) const;

	boost::shared_ptr<Box<> > slicesBoundingBox(const boost::shared_ptr<Slices> slices);
	
	bool checkBlockSlices(const boost::shared_ptr<Blocks> sliceBlocks,
						  const boost::shared_ptr<Blocks> needBlocks);
	
	pipeline::Value<Features> guaranteeFeatures(
							const boost::shared_ptr<SegmentWriter> segmentWriter,
							const boost::shared_ptr<Segments> segments);
	
	boost::shared_ptr<Blocks> segmentBoundingBlocks(const boost::shared_ptr<Segments> segments);

	pipeline::Input<SegmentStore> _segmentStore;
	pipeline::Input<SliceStore> _sliceStore;
	pipeline::Input<Blocks> _blocks;
	pipeline::Input<StackStore> _rawImageStore;

	pipeline::Output<Blocks> _needBlocks;
};

#endif //SEGMENT_GUARANTOR_H__
