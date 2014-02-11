#include "SegmentWriter.h"

SegmentWriter::SegmentWriter()
{
	registerInput(_segments, "segments");
	registerInput(_blocks, "blocks");
	registerInput(_store, "store");
	
	registerOutput(_result, "count");
}


void SegmentWriter::updateOutputs()
{
	boost::shared_ptr<SegmentStoreResult> result = boost::make_shared<SegmentStoreResult>();
	
	foreach(boost::shared_ptr<Block> block, *_blocks)
	{
		foreach (boost::shared_ptr<Segment> segment, _segments->getSegments())
		{
			if (associated(segment, block))
			{
				_store->associate(segment, block);
				result->count += 1;
			}
		}
	}
	
	*_result = *result;
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
