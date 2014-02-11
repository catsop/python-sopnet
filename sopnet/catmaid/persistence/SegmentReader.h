#ifndef SEGMENT_READER_H__
#define SEGMENT_READER_H__

#include <pipeline/all.h>
#include <sopnet/block/Box.h>
#include <sopnet/block/Block.h>
#include <sopnet/block/Blocks.h>
#include <sopnet/block/BlockManager.h>
#include <sopnet/segments/Segment.h>
#include <sopnet/segments/Segments.h>
#include <catmaid/persistence/SegmentStore.h>

class SegmentReader : public pipeline::SimpleProcessNode<>
{
public:
	SegmentReader();
	
private:
	void updateOutputs();
	
	void onBoxSet(const pipeline::InputSetBase&);
	
	void onBlocksSet(const pipeline::InputSetBase&);
	
	pipeline::Input<Blocks> _blocks;
	pipeline::Input<Box<> > _box;
	pipeline::Input<BlockManager> _blockManager;
	pipeline::Input<SegmentStore> _store;
	
	pipeline::Output<Segments> _segments;
	
	bool _sourceIsBox;
};

#endif //SEGMENT_READER_H__
