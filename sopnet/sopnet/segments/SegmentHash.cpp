#include "SegmentHash.h"
#include <boost/functional/hash.hpp>
#include <algorithm>

size_t
SegmentHash::generate(const Segment& segment) {

	std::vector<size_t> sliceHashes;

	foreach (boost::shared_ptr<Slice> slice, segment.getSlices())
		sliceHashes.push_back(slice->hashValue());

	return SegmentHash::generate(sliceHashes);
}

size_t
SegmentHash::generate(std::vector<std::size_t> sliceHashes) {

	std::size_t hash = 0;

	std::sort(sliceHashes.begin(), sliceHashes.end());

	foreach(std::size_t sliceHash, sliceHashes)
		boost::hash_combine(hash, sliceHash);

	return hash;
}
