#include "SegmentReader.h"
#include <catmaid/persistence/SegmentPointerHash.h>

#include <boost/unordered_set.hpp>

#include <util/Logger.h>

logger::LogChannel segmentreaderlog("segmentreaderlog", "[SegmentReader] ");

SegmentReader::SegmentReader()
{
	registerInput(_box, "box", pipeline::Optional);
	registerInput(_blocks, "blocks", pipeline::Optional);
	registerInput(_store, "store");
	registerInput(_blockManager, "block manager");
	registerOutput(_segments, "segments");
	
	_box.registerBackwardCallback(&SegmentReader::onBoxSet, this);
	_blocks.registerBackwardCallback(&SegmentReader::onBlocksSet, this);
}

void
SegmentReader::onBoxSet(const pipeline::InputSetBase& )
{
	_sourceIsBox = true;
}

void
SegmentReader::onBlocksSet(const pipeline::InputSetBase& )
{
	_sourceIsBox = false;
	setInput("block manager", _blocks->getManager());
}

void
SegmentReader::updateOutputs()
{
	SegmentSet segmentSet;
	boost::shared_ptr<Segments> segments = boost::make_shared<Segments>();
	boost::shared_ptr<Blocks> blocks;

	if (!_blocks && !_box)
	{
		LOG_ERROR(segmentreaderlog) << "Need either box or blocks, neither was set" << std::endl;
		*_segments = *segments;
		return;
	}
	else if (_sourceIsBox)
	{
		LOG_ERROR(segmentreaderlog) << "Blocks derived from box input" << std::endl;
		blocks = _blockManager->blocksInBox(_box);
	}
	else
	{
		LOG_ERROR(segmentreaderlog) << "Using blocks input directly" << std::endl;
		blocks = _blocks;
	}
	
	foreach (boost::shared_ptr<Block> block, *blocks)
	{
		boost::shared_ptr<Segments> blockSegments = _store->retrieveSegments(block);
		foreach (boost::shared_ptr<Segment> segment, blockSegments->getSegments())
		{
			if (!segmentSet.count(segment))
			{
				segments->add(segment);
				segmentSet.insert(segment);
			}
		}
	}
	
	*_segments = *segments;
}
