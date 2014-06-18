#include "SegmentFeatureReader.h"
#include <util/Logger.h>
#include <sopnet/features/SegmentFeaturesExtractor.h>

logger::LogChannel segmentfeaturereaderlog("segmentfeaturereaderlog", "[SegmentFeatureReader] ");

SegmentFeatureReader::SegmentFeatureReader()
{
	registerInput(_segments, "segments");
	registerInput(_store, "store");
	registerInput(_storedOnly, "stored only", pipeline::Optional);
	registerInput(_blockManager, "block manager", pipeline::Optional);
	registerInput(_rawStackStore, "raw stack store", pipeline::Optional);
	registerOutput(_features, "features");
}

void SegmentFeatureReader::updateOutputs()
{
	bool storedOnly = (_storedOnly.isSet() && *_storedOnly);
	boost::shared_ptr<Features> features;
	boost::shared_ptr<Segments> featurelessSegments = boost::make_shared<Segments>();
	
	LOG_DEBUG(segmentfeaturereaderlog) << "Reconstituting features" << std::endl;
	
	features = reconstituteFeatures(featurelessSegments);
	
	LOG_DEBUG(segmentfeaturereaderlog) << featurelessSegments->size() << " of " <<
		_segments->size() << " segments had no stored features" << std::endl;
	
	if (!storedOnly && featurelessSegments->size() > 0)
	{
		if (_blockManager.isSet() && _rawStackStore.isSet())
		{
			LOG_DEBUG(segmentfeaturereaderlog) << "Assembling features" << std::endl;
			features = assembleFeatures(features, featurelessSegments);
		}
		else
		{
			LOG_ERROR(segmentfeaturereaderlog) <<
				"Check failed due to absent inputs. Could not extract features" << std::endl;
			return;
		}
	}
	
	_features = new Features();
	*_features = *features;
}

boost::shared_ptr<Features>
SegmentFeatureReader::reconstituteFeatures(const boost::shared_ptr<Segments> featurelessSegments)
{
	boost::shared_ptr<Features> features = boost::make_shared<Features>();
	pipeline::Value<SegmentStore::SegmentFeaturesMap> featureMap;
	std::vector<std::string> featureNames;
	std::map<unsigned int, unsigned int> idMap;
	
	boost::unordered_set<boost::shared_ptr<Segment>,
		SegmentPointerHash, SegmentPointerEquals> segmentTestSet;
	
	unsigned int i = 0;

	featureMap = _store->retrieveFeatures(_segments);
	featureNames = _store->getFeatureNames();

	features->resize(featureMap->size(), featureNames.size());
	
	foreach (std::string name, featureNames)
	{
		features->addName(name);
	}
	
	foreach (boost::shared_ptr<Segment> segment, _segments->getSegments())
	{
		if (featureMap->count(segment))
		{
			idMap[segment->getId()] = i;
			(*features)[i] = (*featureMap)[segment];
			++i;
		}
		else
		{
			LOG_ALL(segmentfeaturereaderlog) << "Segment with id: " << segment->getId() <<
				" not found in feature map" << std::endl;
			if (featurelessSegments)
			{
				featurelessSegments->add(segment);
			}
		}
	}
	
	features->setSegmentIdsMap(idMap);
	
	return features;
}

boost::shared_ptr<Features>
SegmentFeatureReader::assembleFeatures(const boost::shared_ptr<Features> storedFeatures,
									   const boost::shared_ptr<Segments> segments)
{
	boost::shared_ptr<Box<> > boundingBox = segments->boundingBox();
	boost::shared_ptr<Blocks> boundingBlocks = _blockManager->blocksInBox(boundingBox);
	
	LOG_DEBUG(segmentfeaturereaderlog) << "Bounding box is " << *boundingBox << std::endl;
	LOG_DEBUG(segmentfeaturereaderlog) << "Bounding blocks: " << *boundingBlocks <<
		" with " << boundingBlocks->length() << " blocks" << std::endl;
	
	boost::shared_ptr<Features> extractedFeatures = extractFeatures(segments, boundingBlocks);
	
	// At this point, we have both sets of Features. Now we need to merge them together.
	boost::shared_ptr<Features> assembledFeatures = boost::make_shared<Features>();
	
	std::map<unsigned int, unsigned int> assembledSegmentIdsMap;
	
	// Setup assembled features
	assembledFeatures->resize(storedFeatures->size() + extractedFeatures->size(),
							  storedFeatures->getNames().size());
	
	foreach (std::string name, storedFeatures->getNames())
	{
		assembledFeatures->addName(name);
	}

	appendFeatures(assembledFeatures, assembledSegmentIdsMap, storedFeatures);
	appendFeatures(assembledFeatures, assembledSegmentIdsMap, extractedFeatures);
	
	assembledFeatures->setSegmentIdsMap(assembledSegmentIdsMap);
	
	return assembledFeatures;
}

void
SegmentFeatureReader::appendFeatures(const boost::shared_ptr<Features> outFeatures,
									 std::map<unsigned int, unsigned int>& outSegmentIdsMap,
									 const boost::shared_ptr<Features> inFeatures)
{
	// Assume that the keys in outSegmentIdsMap are 0..size() - 1
	// Assume that Features sets are non-overlapping
	int idx = outSegmentIdsMap.size();
	std::map<unsigned int, unsigned int> inSegmentIdsMap = inFeatures->getSegmentsIdsMap();
	std::map<unsigned int, unsigned int>::iterator it;
	// Maps are Segment::getId() -> linear index
	
	for (it = inSegmentIdsMap.begin(); it != inSegmentIdsMap.end(); ++it)
	{
		unsigned int inId = it->first; // Segment id
		unsigned int inIdx = it->second; // linear index
		outSegmentIdsMap[inId] = idx;
		(*outFeatures)[idx] = (*inFeatures)[inIdx];
		++idx;
	}
}


boost::shared_ptr<Features>
SegmentFeatureReader::extractFeatures(const boost::shared_ptr<Segments> segments,
									  const boost::shared_ptr<Blocks> boundingBlocks)
{
	boost::shared_ptr<SegmentFeaturesExtractor> segmentFeaturesExtractor = 
		boost::make_shared<SegmentFeaturesExtractor>();
	boost::shared_ptr<Features> extractedFeaturesPtr;
	
	pipeline::Value<Features> extractedFeatures;
	
	util::point3<unsigned int> offset = boundingBlocks->location();
	
	pipeline::Value<util::point3<unsigned int> > valueOffset(offset);

	segmentFeaturesExtractor->setInput("segments", segments);
	segmentFeaturesExtractor->setInput("raw sections",
									   _rawStackStore->getImageStack(*boundingBlocks));
	segmentFeaturesExtractor->setInput("crop offset", valueOffset);
	extractedFeatures = segmentFeaturesExtractor->getOutput("all features");

	// There is a better way to do this.
	extractedFeaturesPtr = extractedFeatures;
	
	return extractedFeaturesPtr;
}
