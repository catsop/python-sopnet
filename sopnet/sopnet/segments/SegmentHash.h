#ifndef SOPNET_SEGMENTS_SEGMENT_HASH_H__
#define SOPNET_SEGMENTS_SEGMENT_HASH_H__

#include "Segment.h"

struct SegmentHash {

	static std::size_t generate(const Segment& segment);

	static std::size_t generate(std::vector<std::size_t> sliceHashes);
};

#endif // SOPNET_SEGMENTS_SEGMENT_HASH_H__

