#ifndef SOPNET_CATMAID_PERSISTENCE_SEGMENT_DESCRIPTION_H__
#define SOPNET_CATMAID_PERSISTENCE_SEGMENT_DESCRIPTION_H__

#include <cstddef>
#include <vector>

/**
 * A lightweight representation of a segment, as represented in the database 
 * backends. Instead of storing slices, only the hashes of the slices are 
 * stored.
 */
class SegmentDescription {

public:

	SegmentDescription(unsigned int section) :
		_hashDirty(true),
		_section(section) {}

	std::size_t getHash();

	unsigned int getSection() { return _section; }

	void setFeatures(const std::vector<double>& features) { _features = features; }

	const std::vector<double>& getFeatures() const { return _features; }

	void addLeftSlice(std::size_t sliceHash) { _hashDirty = true; _leftSliceHashes.push_back(sliceHash); }

	void addRightSlice(std::size_t sliceHash) { _hashDirty = true; _leftSliceHashes.push_back(sliceHash); }

	const std::vector<std::size_t>& getLeftSlices() { return _leftSliceHashes; }

	const std::vector<std::size_t>& getRightSlices() { return _rightSliceHashes; }

private:

	// the hash of this segment
	std::size_t _hash;
	bool _hashDirty;

	// hashes of the slices to the left and right
	std::vector<std::size_t> _leftSliceHashes;
	std::vector<std::size_t> _rightSliceHashes;

	// the features of this segment
	std::vector<double> _features;

	// the lower of the two sections this segment lives in
	unsigned int _section;
};

#endif // SOPNET_CATMAID_PERSISTENCE_SEGMENT_DESCRIPTION_H__

