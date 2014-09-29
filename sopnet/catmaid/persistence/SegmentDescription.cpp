#include "SegmentDescription.h"
#include <util/foreach.h>

SegmentHash
SegmentDescription::getHash() const {

	if (_hashDirty) {

		std::vector<SliceHash> sliceHashes;

		foreach (SliceHash hash, _leftSliceHashes)
			sliceHashes.push_back(hash);
		foreach (SliceHash hash, _rightSliceHashes)
			sliceHashes.push_back(hash);

		_hash = hash_value(sliceHashes);
		_hashDirty = false;
	}

	return _hash;
}
