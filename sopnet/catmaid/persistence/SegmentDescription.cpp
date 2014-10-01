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

SegmentType
SegmentDescription::getType() const {

	std::size_t leftSize = _leftSliceHashes.size();
	std::size_t rightSize = _rightSliceHashes.size();

	if (leftSize == 0 || rightSize == 0) return EndSegmentType;
	else if (leftSize == 1 && rightSize == 1) return ContinuationSegmentType;
	else return BranchSegmentType;
}
