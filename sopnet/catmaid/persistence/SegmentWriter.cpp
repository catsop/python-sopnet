#include "SegmentWriter.h"

logger::LogChannel segmentwriterlog("segmentwriterlog", "[SegmentWriter] ");

SegmentWriter::SegmentWriter()
{
	registerInput(_segments, "segments");
	registerInput(_blocks, "blocks");
	registerInput(_features, "features", pipeline::Optional);
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
	
	if (_features.isSet())
	{
		LOG_DEBUG(segmentwriterlog) << "storing features: " << *_features << std::endl;
		_store->storeFeatures(_features);
	}
}

bool
SegmentWriter::associated(const boost::shared_ptr<Segment>& segment,
						  const boost::shared_ptr<Block>& block)
{
	util::rect<unsigned int> blockRect = *block;
	foreach (boost::shared_ptr<Slice> slice, segment->getSlices())
	{
		if (blockRect.intersects(
			static_cast<util::rect<unsigned int> >(slice->getComponent()->getBoundingBox())))
		{
			return true;
		}
	}
	
	return false;
}
