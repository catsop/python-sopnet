#ifndef SEGMENT_WRITER_H__
#define SEGMENT_WRITER_H__

#include <pipeline/all.h>
#include <sopnet/segments/Segments.h>
#include <sopnet/segments/Segment.h>
#include <sopnet/block/Block.h>
#include <sopnet/block/Blocks.h>
#include <catmaid/persistence/SegmentStore.h>

class SegmentWriter : public pipeline::SimpleProcessNode<>
{
public:
	SegmentWriter();
	
private:
	void updateOutputs();
	bool associated(const boost::shared_ptr<Segment>& segment,
					const boost::shared_ptr<Block>& block);
	
	pipeline::Input<Segments> _segments;
	pipeline::Input<Blocks> _blocks;
	pipeline::Input<SegmentStore> _store;

	pipeline::Output<SegmentStoreResult> _result;
};

#endif //SEGMENT_WRITER_H__
