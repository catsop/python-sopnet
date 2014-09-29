#ifndef SOPNET_SEGMENTS_SEGMENT_HASH_H__
#define SOPNET_SEGMENTS_SEGMENT_HASH_H__

#include <cstddef>
#include <slices/SliceHash.h>
#include "Segment.h"

typedef std::size_t SegmentHash;

SegmentHash hash_value(const Segment& segment);

SegmentHash hash_value(std::vector<SliceHash> sliceHashes);

#endif // SOPNET_SEGMENTS_SEGMENT_HASH_H__

