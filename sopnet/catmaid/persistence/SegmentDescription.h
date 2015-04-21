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
			const util::rect<unsigned int>& boundingBox) :
		_hashDirty(true),
		_cost(std::numeric_limits<double>::signaling_NaN()),
		_section(section),
		_boundingBox(boundingBox) {}

	SegmentDescription(const Segment& segment);

	SegmentHash getHash() const;

	/**
	 * Get the higher number of the two sections that this segment is connecting.
	 */
	unsigned int getSection() const { return _section; }

	const util::rect<unsigned int>& get2DBoundingBox() const { return _boundingBox; }

	void setFeatures(const std::vector<double>& features) { _features = features; }

	const std::vector<double>& getFeatures() const { return _features; }

	void setCost(double cost) { _cost = cost; }

	double getCost() const { return _cost; }

	/**
	 * Add a slice to the left (i.e., lower) side of this segment.
	 */
	void addLeftSlice(SliceHash sliceHash) { _hashDirty = true; _leftSliceHashes.push_back(sliceHash); }

	/**
	 * Add a slice to the right (i.e., higher) side of this segment.
	 */
	void addRightSlice(SliceHash sliceHash) { _hashDirty = true; _rightSliceHashes.push_back(sliceHash); }

	/**
	 * Get the slices of the left (i.e., lower) side of this segment.
	 */
	const std::vector<SliceHash>& getLeftSlices() const { return _leftSliceHashes; }

	/**
	 * Get the slices of the right (i.e., higher) side of this segment.
	 */
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

	// the cost of this segment
	double _cost;

	// the higher of the two sections this segment lives in
	unsigned int _section;

	// the bounding box of the segment in 2D
	util::rect<unsigned int> _boundingBox;
};

#endif // SOPNET_CATMAID_PERSISTENCE_SEGMENT_DESCRIPTION_H__

