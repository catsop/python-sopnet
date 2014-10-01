#ifndef SOPNET_CATMAID_PERSISTENCE_SEGMENT_DESCRIPTION_H__
#define SOPNET_CATMAID_PERSISTENCE_SEGMENT_DESCRIPTION_H__

#include <cstddef>
#include <vector>
#include <util/rect.hpp>
#include <segments/Segment.h>
#include <segments/SegmentHash.h>
#include <slices/SliceHash.h>

/**
 * A lightweight representation of a segment, as represented in the database 
 * backends. Instead of storing slices, only the hashes of the slices are 
 * stored.
 */
class SegmentDescription {

public:

	SegmentDescription(
			unsigned int section,
			const util::rect<unsigned int>& boundingBox,
			const util::point<double> center) :
		_hashDirty(true),
		_section(section),
		_boundingBox(boundingBox),
		_center(center) {}

	SegmentHash getHash() const;

	unsigned int getSection() const { return _section; }

	const util::rect<unsigned int>& get2DBoundingBox() const { return _boundingBox; }

	const util::point<double>& getCenter() const { return _center; }

	void setFeatures(const std::vector<double>& features) { _features = features; }

	const std::vector<double>& getFeatures() const { return _features; }

	void addLeftSlice(SliceHash sliceHash) { _hashDirty = true; _leftSliceHashes.push_back(sliceHash); }

	void addRightSlice(SliceHash sliceHash) { _hashDirty = true; _leftSliceHashes.push_back(sliceHash); }

	const std::vector<SliceHash>& getLeftSlices() const { return _leftSliceHashes; }

	const std::vector<SliceHash>& getRightSlices() const { return _rightSliceHashes; }

	SegmentType getType() const;

private:

	// the hash of this segment
	mutable SegmentHash _hash;
	mutable bool _hashDirty;

	// hashes of the slices to the left and right
	std::vector<SliceHash> _leftSliceHashes;
	std::vector<SliceHash> _rightSliceHashes;

	// the features of this segment
	std::vector<double> _features;

	// the lower of the two sections this segment lives in
	unsigned int _section;

	// the bounding box of the segment in 2D
	util::rect<unsigned int> _boundingBox;

	// the 2D center of this segment in the inter-section interval
	util::point<double> _center;
};

#endif // SOPNET_CATMAID_PERSISTENCE_SEGMENT_DESCRIPTION_H__

