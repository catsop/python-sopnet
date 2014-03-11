#ifndef SEGMENT_GUARANTOR_H__
#define SEGMENT_GUARANTOR_H__

#include <pipeline/all.h>
#include <catmaid/SliceGuarantor.h>
#include <catmaid/persistence/SegmentStore.h>
#include <sopnet/block/BlockManager.h>
#include <catmaid/persistence/SliceReader.h>
#include <catmaid/persistence/SegmentWriter.h>
#include <catmaid/persistence/StackStore.h>

class SegmentGuarantor : public pipeline::SimpleProcessNode<>
{
public:
	SegmentGuarantor();
	
	
	pipeline::Value<Blocks> guaranteeSegments();
	
private:
	void updateOutputs();

	boost::shared_ptr<Slices> collectSlicesByZ(const boost::shared_ptr<Slices> slices,
											   unsigned int z) const;

	boost::shared_ptr<Box<> > boundingBox(const boost::shared_ptr<Slices> slices);
	
	bool checkBlockSlices(const boost::shared_ptr<Blocks> sliceBlocks,
						  const boost::shared_ptr<Blocks> needBlocks);
	
	void guaranteeFeatures(const boost::shared_ptr<SegmentWriter> segmentWriter,
							const boost::shared_ptr<Segments> segments,
							const boost::shared_ptr<Blocks> blocks);

	pipeline::Input<SegmentStore> _segmentStore;
	pipeline::Input<SliceStore> _sliceStore;
	pipeline::Input<Blocks> _blocks;
	pipeline::Input<StackStore> _rawImageStore;

	pipeline::Output<Blocks> _needBlocks;
};

#endif //SEGMENT_GUARANTOR_H__
