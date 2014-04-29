#ifndef SEGMENT_READER_H__
#define SEGMENT_READER_H__

#include <pipeline/all.h>
#include <sopnet/block/Box.h>
#include <sopnet/block/Block.h>
#include <sopnet/block/Blocks.h>
#include <sopnet/block/CoreManager.h>
#include <sopnet/segments/Segment.h>
#include <sopnet/segments/Segments.h>
#include <catmaid/persistence/SegmentStore.h>

class SegmentReader : public pipeline::SimpleProcessNode<>
{
public:
	/**
	 * Construct a SegmentReader
	 * Inputs:
	 *   Blocks "blocks"
	 *   SegmentStore "store"
	 * Outputs:
	 *   Segments "segments"
	 */
	SegmentReader();
	
private:
	void updateOutputs();
	
	pipeline::Input<Blocks> _blocks;
	pipeline::Input<SegmentStore> _store;
	
	pipeline::Output<Segments> _segments;
};

#endif //SEGMENT_READER_H__
