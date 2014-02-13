#include "SegmentReader.h"
#include <catmaid/persistence/SegmentPointerHash.h>

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

	segments = _store->retrieveSegments(_blocks);

	*_segments = *segments;
}
