#include "SegmentHash.h"
#include <boost/functional/hash.hpp>
#include <algorithm>

SegmentHash
hash_value(const Segment& segment) {

	std::vector<SliceHash> sliceHashes;

	foreach (boost::shared_ptr<Slice> slice, segment.getSlices())
		sliceHashes.push_back(slice->hashValue());

	return hash_value(sliceHashes);
}

SegmentHash
hash_value(std::vector<SliceHash> sliceHashes) {

	SliceHash hash = 0;

	std::sort(sliceHashes.begin(), sliceHashes.end());

	foreach(SliceHash sliceHash, sliceHashes)
		boost::hash_combine(hash, sliceHash);

	return hash;
}
