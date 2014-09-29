#ifndef SOPNET_CATMAID_PERSISTENCE_SEGMENT_DESCRIPTION_H__
#define SOPNET_CATMAID_PERSISTENCE_SEGMENT_DESCRIPTION_H__

#include <cstddef>
#include <vector>
#include <util/rect.hpp>

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
		_section(section),
		_boundingBox(boundingBox) {}

	std::size_t getHash() const;

	unsigned int getSection() const { return _section; }

	const util::rect<unsigned int>& get2DBoundingBox() const { return _boundingBox; }

	void setFeatures(const std::vector<double>& features) { _features = features; }

	const std::vector<double>& getFeatures() const { return _features; }

	void addLeftSlice(std::size_t sliceHash) { _hashDirty = true; _leftSliceHashes.push_back(sliceHash); }

	void addRightSlice(std::size_t sliceHash) { _hashDirty = true; _leftSliceHashes.push_back(sliceHash); }

	const std::vector<std::size_t>& getLeftSlices() const { return _leftSliceHashes; }

	const std::vector<std::size_t>& getRightSlices() const { return _rightSliceHashes; }

private:

	// the hash of this segment
	mutable std::size_t _hash;
	mutable bool _hashDirty;

	// hashes of the slices to the left and right
	std::vector<std::size_t> _leftSliceHashes;
	std::vector<std::size_t> _rightSliceHashes;

	// the features of this segment
	std::vector<double> _features;

	// the lower of the two sections this segment lives in
	unsigned int _section;

	// the bounding box of the segment in 2D
	util::rect<unsigned int> _boundingBox;
};

#endif // SOPNET_CATMAID_PERSISTENCE_SEGMENT_DESCRIPTION_H__

