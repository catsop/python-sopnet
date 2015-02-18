#include "SegmentDescription.h"
#include <util/foreach.h>
#include <imageprocessing/ConnectedComponent.h>


SegmentDescription::SegmentDescription(const Segment& segment) :
		_hashDirty(true),
		_cost(std::numeric_limits<double>::signaling_NaN()),
		_section(segment.getInterSectionInterval()),
		_boundingBox(0, 0, 0, 0),
		_center(segment.getCenter()) {

	// get the 2D bounding box of the segment
	foreach (boost::shared_ptr<Slice> slice, segment.getSlices()) {

		if (_boundingBox.area() == 0)
			_boundingBox = slice->getComponent()->getBoundingBox();
		else
			_boundingBox.fit(slice->getComponent()->getBoundingBox());
	}

	// add slice hashes
	if (segment.getDirection() == Left) {

		foreach (boost::shared_ptr<Slice> slice, segment.getTargetSlices())
			addLeftSlice(slice->hashValue());
		foreach (boost::shared_ptr<Slice> slice, segment.getSourceSlices())
			addRightSlice(slice->hashValue());

	} else {

		foreach (boost::shared_ptr<Slice> slice, segment.getTargetSlices())
			addRightSlice(slice->hashValue());
		foreach (boost::shared_ptr<Slice> slice, segment.getSourceSlices())
			addLeftSlice(slice->hashValue());
	}
}

SegmentHash
SegmentDescription::getHash() const {

	if (_hashDirty) {
		_hash = hash_value(_leftSliceHashes, _rightSliceHashes);

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
