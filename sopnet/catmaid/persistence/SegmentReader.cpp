#include "SegmentReader.h"

#include <boost/unordered_set.hpp>
#include <util/Logger.h>

logger::LogChannel segmentreaderlog("segmentreaderlog", "[SegmentReader] ");

SegmentReader::SegmentReader()
{
	registerInput(_blocks, "blocks");
	registerInput(_store, "store");
	registerOutput(_segments, "segments");
	registerOutput(_features, "features");
}

void
SegmentReader::updateOutputs()
{
	pipeline::Value<Segments> segments;


	LOG_DEBUG(segmentreaderlog) << "Reading segments from " << *_blocks << std::endl;
	segments = _store->retrieveSegments(_blocks);
	LOG_DEBUG(segmentreaderlog) << "Read " << segments->size() << " segments" << std::endl;
	
	*_features = *(reconstituteFeatures(segments));
	*_segments = *segments;
}


pipeline::Value<Features>
SegmentReader::reconstituteFeatures(pipeline::Value<Segments> segments)
{
	pipeline::Value<Features> features;
	pipeline::Value<SegmentStore::SegmentFeaturesMap> featureMap;
	SegmentStore::SegmentFeaturesMap::iterator it;
	std::vector<std::string> featureNames;
	std::map<unsigned int, unsigned int> idMap;
	
	unsigned int i = 0;

	featureMap = _store->retrieveFeatures(segments);
	featureNames = _store->getFeatureNames();
	
	foreach (std::string name, featureNames)
	{
		features->addName(name);
	}
	
	for (it = featureMap->begin(); it != featureMap->end(); ++it)
	{
		idMap[i] = it->first->getId();
		(*features)[i] = it->second;
		++i;
	}
	
	features->setSegmentIdsMap(idMap);
	
	return features;
}
