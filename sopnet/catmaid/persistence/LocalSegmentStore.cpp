#include "LocalSegmentStore.h"
#include <map>
#include <vector>
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
	std::vector<boost::shared_ptr<Segment> > segmentVector;
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
				segmentSet.add(segment);
			}
		}
		else
		{
			LOG_DEBUG(localsegmentstorelog) << "Block " << *block <<
				" was requested, but doesn't exist in the map" << std::endl;
		}
	}
	
	LOG_DEBUG(localsegmentstorelog) << "Retrieved " << segmentSet.size() << " unique segments" << std::endl;
	
	// Sort the segments.
	segmentVector.insert(segmentVector.begin(), segmentSet.begin(), segmentSet.end());
	std::sort(segmentVector.begin(), segmentVector.end(), LocalSegmentStore::compareSegments);
	
	
	foreach (boost::shared_ptr<Segment> segment, segmentVector)
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
	if (_segmentMasterList.find(segment))
	{
		unsigned int existingId = _segmentMasterList.find(segment)->getId();
		_idSegmentMap[segment->getId()] = _idSegmentMap[existingId];
	}
	else
	{
		_idSegmentMap[segment->getId()] = segment;
		_segmentMasterList.add(segment);
	}
}

boost::shared_ptr<Segment>
LocalSegmentStore::equivalentSegment(const boost::shared_ptr<Segment>& segment)
{
	if (_segmentMasterList.contains(segment))
	{
		unsigned int id = _segmentMasterList.find(segment)->getId();
		boost::shared_ptr<Segment> eqSegment = _idSegmentMap[id];
		return eqSegment;
	}
	return segment;
}

void LocalSegmentStore::dumpStore()
{
	foreach (boost::shared_ptr<Segment> segment, _segmentMasterList)
	{
		LOG_ALL(localsegmentstorelog) << segment->getId();
		
		foreach (boost::shared_ptr<Slice> slice, segment->getSourceSlices())
		{
			LOG_ALL(localsegmentstorelog) << " " << slice->getId();
		}
		
		LOG_ALL(localsegmentstorelog) << " :";
		
		foreach (boost::shared_ptr<Slice> slice, segment->getTargetSlices())
		{
			LOG_ALL(localsegmentstorelog) << " " << slice->getId();
		}
		
		LOG_ALL(localsegmentstorelog) << std::endl;
	}
}

int
LocalSegmentStore::storeFeatures(pipeline::Value<Features> features)
{
	std::map<unsigned int, unsigned int>::const_iterator it;
	std::map<unsigned int, unsigned int> idMap = features->getSegmentsIdsMap();
	int count = 0;
	
	for (it = idMap.begin(); it != idMap.end(); ++it)
	{
		unsigned int id = it->second;
		unsigned int i = it->first;
		if (_idSegmentMap.count(id))
		{
			boost::shared_ptr<Segment> segment = _idSegmentMap[id];
			_featureMasterMap[segment] = (*features)[i];
			++count;
		}
	}
	
	if (count > 0 && _featureNames.empty())
	{
		foreach (std::string name, features->getNames())
		{
			// For some reason, vector.insert wouldn't work here.
			_featureNames.push_back(name);
		}
	}

	LOG_DEBUG(localsegmentstorelog) << "Wrote features for " << count << " of " <<
		features->size() << " segments" << std::endl;
	return count;
}

pipeline::Value<SegmentStore::SegmentFeaturesMap>
LocalSegmentStore::retrieveFeatures(pipeline::Value<Segments> segments)
{
	pipeline::Value<SegmentStore::SegmentFeaturesMap> featureMap;
	
	foreach (boost::shared_ptr<Segment> segment, segments->getSegments())
	{
		if (_featureMasterMap.count(segment))
		{
			std::vector<double> features = _featureMasterMap[segment];
			(*featureMap)[segment] = features;
		}
	}
	
	return featureMap;
}

std::vector<std::string>
LocalSegmentStore::getFeatureNames()
{
	return _featureNames;
}

