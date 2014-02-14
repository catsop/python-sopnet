#include "LocalSegmentStore.h"
#include <util/Logger.h>
logger::LogChannel localsegmentstorelog("localsegmentstorelog", "[LocalSegmentStore] ");

LocalSegmentStore::LocalSegmentStore()
{

}

void
LocalSegmentStore::associate(pipeline::Value<Segments> segmentsIn,
							 pipeline::Value<Block> block)
{
	foreach (boost::shared_ptr<Segment> segmentIn, segmentsIn->getSegments())
	{
		boost::shared_ptr<Segment> segment = equivalentSegment(segmentIn);

		mapBlockToSegment(block, segment);
		mapSegmentToBlock(segment, block);

		addSegmentToMasterList(segment);
	}
}

pipeline::Value<Blocks>
LocalSegmentStore::getAssociatedBlocks(pipeline::Value<Segment> segment)
{
	pipeline::Value<Blocks> blocks;
	
	if (_segmentBlockMap.count(segment))
	{
		blocks->addAll(_segmentBlockMap[segment]);
	}
	
	return blocks;
}

pipeline::Value<Segments>
LocalSegmentStore::retrieveSegments(pipeline::Value<Blocks> blocks)
{
	pipeline::Value<Segments> segments;
	SegmentSet segmentSet;
	
	foreach (boost::shared_ptr<Block> block, *blocks)
	{
		if (_blockSegmentMap.count(*block))
		{
			LOG_DEBUG(localsegmentstorelog) << "Retrieving segments for " << *block << std::endl;
			boost::shared_ptr<Segments> blockSegments = _blockSegmentMap[*block];
			LOG_DEBUG(localsegmentstorelog) << "Retrieved " << blockSegments->size() <<
				" segments" << std::endl;
			foreach (boost::shared_ptr<Segment> segment, blockSegments->getSegments())
			{
				segmentSet.insert(segment);
			}
		}
		else
		{
			LOG_DEBUG(localsegmentstorelog) << "Block " << *block <<
				" was requested, but doesn't exist in the map" << std::endl;
		}
	}
	
	LOG_DEBUG(localsegmentstorelog) << "Retrieved " << segmentSet.size() << " unique segments" << std::endl;
	
	foreach (boost::shared_ptr<Segment> segment, segmentSet)
	{
		segments->add(segment);
	}
	
	LOG_DEBUG(localsegmentstorelog) << "returning" << std::endl;
	
	return segments;
}

boost::shared_ptr<Segments>
LocalSegmentStore::getSegments(const boost::shared_ptr< Block >& block,
	const boost::shared_ptr<Segment>& segment)
{
	boost::shared_ptr<Segments> segments;
	boost::shared_ptr<Segments> nullPtr = boost::shared_ptr<Segments>();
	
	if (_blockSegmentMap.count(*block))
	{
		segments = _blockSegmentMap[*block];
	}
	else
	{
		segments = boost::make_shared<Segments>();
		_blockSegmentMap[*block] = segments;
	}
	
	foreach (boost::shared_ptr<Segment> cSegment, segments->getSegments())
	{
		if (*segment == *cSegment)
		{
			return nullPtr;
		}
	}
	
	return segments;
}


void
LocalSegmentStore::mapBlockToSegment(const boost::shared_ptr<Block>& block,
										  const boost::shared_ptr<Segment>& segment)
{
	boost::shared_ptr<Segments> segments = getSegments(block, segment);
	if (segments)
	{
		segments->add(segment);
	}
}

void
LocalSegmentStore::mapSegmentToBlock(const boost::shared_ptr<Segment>& segment,
									 const boost::shared_ptr<Block>& block)
{
	boost::shared_ptr<Blocks> blocks;
	
	if (_segmentBlockMap.count(segment))
	{
		blocks = _segmentBlockMap[segment];
	}
	else
	{
		blocks = boost::make_shared<Blocks>();
		_segmentBlockMap[segment] = blocks;
	}
	
	foreach (boost::shared_ptr<Block> cBlock, *blocks)
	{
		if (*block == *cBlock)
		{
			return;
		}
	}
	
	blocks->add(block);
}

void
LocalSegmentStore::addSegmentToMasterList(const boost::shared_ptr<Segment>& segment)
{
	if (_segmentMasterList.count(segment))
	{
		unsigned int existingId = (*_segmentMasterList.find(segment))->getId();
		_idSegmentMap[segment->getId()] = _idSegmentMap[existingId];
	}
	else
	{
		_idSegmentMap[segment->getId()] = segment;
		_segmentMasterList.insert(segment);
	}
}

boost::shared_ptr<Segment>
LocalSegmentStore::equivalentSegment(const boost::shared_ptr<Segment>& segment)
{
	if (_segmentMasterList.count(segment))
	{
		unsigned int id = (*_segmentMasterList.find(segment))->getId();
		boost::shared_ptr<Segment> eqSegment = _idSegmentMap[id];
		return eqSegment;
	}
	else
	{
		return segment;
	}
}
