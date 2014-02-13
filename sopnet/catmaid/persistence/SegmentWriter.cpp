#include "SegmentWriter.h"

SegmentWriter::SegmentWriter()
{
	registerInput(_segments, "segments");
	registerInput(_blocks, "blocks");
	registerInput(_store, "store");
}


void SegmentWriter::writeSegments()
{
	updateInputs();
	
	foreach(boost::shared_ptr<Block> block, *_blocks)
	{
		pipeline::Value<Block> valueBlock;
		pipeline::Value<Segments> segments;
		*valueBlock = *block;
		
		foreach (boost::shared_ptr<Segment> segment, _segments->getSegments())
		{
			if (associated(segment, block))
			{
				segments->add(segment);
			}
		}
		
		_store->associate(segments, valueBlock);
	}
}

bool
SegmentWriter::associated(const boost::shared_ptr<Segment>& segment,
						  const boost::shared_ptr<Block>& block)
{
	foreach (boost::shared_ptr<Slice> slice, segment->getSlices())
	{
		if (block->overlaps(slice->getComponent()))
		{
			return true;
		}
	}
	
	return false;
}
