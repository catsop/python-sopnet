#ifndef SEGMENT_WRITER_H__
#define SEGMENT_WRITER_H__

#include <pipeline/all.h>
#include <sopnet/segments/Segments.h>
#include <sopnet/segments/Segment.h>
#include <sopnet/block/Block.h>
#include <sopnet/block/Blocks.h>
#include <catmaid/persistence/SegmentStore.h>
#include <sopnet/features/Features.h>

class SegmentWriter : public pipeline::SimpleProcessNode<>
{
public:
	/**
	 * Construct a SegmentWriter
	 * Inputs:
	 *   Segments "segments"
	 *   Blocks "blocks"
	 *   Features "features", optional
	 *   SegmentStore "store"
	 * 
	 * No outputs.
	 */
	SegmentWriter();

	/**
	 * Write segments and segment features to the SegmentStore and associate the segments
	 * with the given Blocks
	 */
	void writeSegments();
	
private:
	void updateOutputs(){}

	bool associated(const boost::shared_ptr<Segment>& segment,
					const boost::shared_ptr<Block>& block);

	pipeline::Input<Segments> _segments;
	pipeline::Input<Features> _features;
	pipeline::Input<Blocks> _blocks;
	pipeline::Input<SegmentStore> _store;
};

#endif //SEGMENT_WRITER_H__
