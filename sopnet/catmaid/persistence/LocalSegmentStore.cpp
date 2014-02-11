#include "LocalSegmentStore.h"
#include <util/Logger.h>
logger::LogChannel localsegmentstorelog("localsegmentstorelog", "[LocalSegmentStore] ");

LocalSegmentStore::LocalSegmentStore() :
	_segmentBlockMap(boost::make_shared<SegmentBlockMap>()),
	_blockSegmentMap(boost::make_shared<BlockSegmentMap>()),
	_idSegmentMap(boost::make_shared<IdSegmentMap>())
{

}

void
LocalSegmentStore::associate(const boost::shared_ptr<Segment>& segmentIn,
							 const boost::shared_ptr<Block>& block)
{
	boost::shared_ptr<Segment> segment = equivalentSegment(segmentIn);

	mapBlockToSegment(block, segment);
	mapSegmentToBlock(segment, block);
	
	addSegmentToMasterList(segment);
	
}

void
LocalSegmentStore::disassociate(const boost::shared_ptr<Segment>& segment,
								const boost::shared_ptr<Block>& block)
{
	if (_segmentBlockMap->count(segment))
	{
		(*_segmentBlockMap)[segment]->remove(block);
		
		if ((*_segmentBlockMap)[segment]->length() == 0)
		{
			_segmentBlockMap->erase(segment);
		}
	}
	
	if (_blockSegmentMap->count(*block))
	{
		(*_blockSegmentMap)[*block]->remove(segment);
		
		if ((*_blockSegmentMap)[*block]->size() == 0)
		{
			_blockSegmentMap->erase(*block);
		}
	}
}

boost::shared_ptr<Blocks>
LocalSegmentStore::getAssociatedBlocks(const boost::shared_ptr<Segment>& segment)
{
	boost::shared_ptr<Blocks> blocks = boost::make_shared<Blocks>();
	
	if (_segmentBlockMap->count(segment))
	{
		blocks->addAll((*_segmentBlockMap)[segment]);
	}
	
	return blocks;
}

void
LocalSegmentStore::removeSegment(const boost::shared_ptr<Segment>& segment)
{
	boost::shared_ptr<Blocks> blocks = getAssociatedBlocks(segment);
	
	foreach (boost::shared_ptr<Block> block, *blocks)
	{
		disassociate(segment, block);
	}
}

boost::shared_ptr<Segments>
LocalSegmentStore::retrieveSegments(const boost::shared_ptr<Block>& block)
{
	boost::shared_ptr<Segments> segments = boost::make_shared<Segments>();
	
	if (_blockSegmentMap->count(*block))
	{
		segments->addAll((*_blockSegmentMap)[*block]);
	}
	
	return segments;
}

boost::shared_ptr<Segments>
LocalSegmentStore::getSegments(const boost::shared_ptr< Block >& block,
	const boost::shared_ptr<Segment>& segment)
{
	boost::shared_ptr<Segments> segments;
	boost::shared_ptr<Segments> nullPtr = boost::shared_ptr<Segments>();
	
	if (_blockSegmentMap->count(*block))
	{
		segments = (*_blockSegmentMap)[*block];
	}
	else
	{
		segments = boost::make_shared<Segments>();
		(*_blockSegmentMap)[*block] = segments;
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
	
	if (_segmentBlockMap->count(segment))
	{
		blocks = (*_segmentBlockMap)[segment];
	}
	else
	{
		blocks = boost::make_shared<Blocks>();
		(*_segmentBlockMap)[segment] = blocks;
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
LocalSegmentStore::addSegmentToMasterList(const boost::shared_ptr< Segment >& segment)
{
	if (_segmentMasterList.count(segment))
	{
		unsigned int existingId = (*_segmentMasterList.find(segment))->getId();
		(*_idSegmentMap)[segment->getId()] = (*_idSegmentMap)[existingId];
	}
	else
	{
		(*_idSegmentMap)[segment->getId()] = segment;
		_segmentMasterList.insert(segment);
	}
}

boost::shared_ptr<Segment>
LocalSegmentStore::equivalentSegment(const boost::shared_ptr<Segment>& segment)
{
	if (_segmentMasterList.count(segment))
	{
		unsigned int id = (*_segmentMasterList.find(segment))->getId();
		boost::shared_ptr<Segment> eqSegment = (*_idSegmentMap)[id];
		return eqSegment;
	}
	else
	{
		return segment;
	}
}
