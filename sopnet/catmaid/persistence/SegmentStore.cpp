#include "SegmentStore.h"

bool SegmentStore::associated(const Segment& segment, const Block& block)
{
	util::rect<unsigned int> blockRect = block;
	foreach (boost::shared_ptr<Slice> slice, segment.getSlices())
	{
		if (blockRect.intersects(
			static_cast<util::rect<unsigned int> >(slice->getComponent()->getBoundingBox())))
		{
			return true;
		}
	}

	return false;
}

void SegmentStore::writeSegments(const Segments& segments, const Blocks& blocks)
{
	foreach(boost::shared_ptr<Block> block, blocks)
	{
		Segments blockSegments;

		foreach (boost::shared_ptr<Segment> segment, segments.getSegments())
		{
			if (associated(*segment, *block))
			{
				blockSegments.add(segment);
			}
		}

		associate(segments, *block);
	}
}
