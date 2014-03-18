#include "SegmentReader.h"

#include <boost/unordered_set.hpp>
#include <util/Logger.h>

logger::LogChannel segmentreaderlog("segmentreaderlog", "[SegmentReader] ");

SegmentReader::SegmentReader()
{
	registerInput(_blocks, "blocks");
	registerInput(_store, "store");
	registerOutput(_segments, "segments");
}

void
SegmentReader::updateOutputs()
{
	pipeline::Value<Segments> segments;
	pipeline::Value<Features> features;

	LOG_DEBUG(segmentreaderlog) << "Reading segments from " << *_blocks << std::endl;
	segments = _store->retrieveSegments(_blocks);
	LOG_DEBUG(segmentreaderlog) << "Read " << segments->size() << " segments" << std::endl;
	
	*_segments = *segments;
}