#ifndef CELLTRACKER_TRACKLET_H__
#define CELLTRACKER_TRACKLET_H__

#include <boost/thread.hpp>

#include <pipeline/all.h>
#include <slices/Slice.h>
#include <util/point.hpp>
#include <util/Hashable.h>
#include "SegmentHash.h"

/**
 * The direction of the segment.
 */
enum Direction {

	Left,
	Right
};

enum SegmentType {
	
	EndSegmentType,
	ContinuationSegmentType,
	BranchSegmentType,
	BaseSegmentType
};

/**
 * A segment represents the connection of slices between sections. Subclasses
 * implement one-to-one segments (continuations), one-to-two segments
 * (branches), and one-to-zero segments (ends).
 */
class Segment : public pipeline::Data, public Hashable<Segment, SegmentHash> {

public:

	/**
	 * Create a new segment.
	 */
	Segment(unsigned int id, Direction direction, const util::point<double, 2>& center, unsigned int interSectionInterval);

	/**
	 * Get the id of this segment.
	 */
	unsigned int getId() const;

	/**
	 * Get the direction of this segment in the inter-segment interval.
	 */
	Direction getDirection() const;

	/**
	 * Get the 2D center of gravity of this segment.
	 */
	const util::point<double, 2>& getCenter() const { return _center; }

	/**
	 * Get the inter-section interval this segment is spanning over.
	 */
	unsigned int getInterSectionInterval() const;

	/**
	 * Get the next available segment id.
	 */
	static unsigned int getNextSegmentId();
	
	static std::string typeString(const SegmentType type);

	virtual std::vector<boost::shared_ptr<Slice> > getSlices() const = 0;

	std::vector<boost::shared_ptr<Slice> > getSourceSlices() const;

	std::vector<boost::shared_ptr<Slice> > getTargetSlices() const;
	
	bool operator== (const Segment& other) const;
	
	bool operator< (const Segment& other) const {return _id < other._id; }
	
	virtual SegmentType getType() const = 0;

private:

	static unsigned int NextSegmentId;

	static boost::mutex SegmentIdMutex;

	// a unique id for the segment
	unsigned int _id;

	// the direction of the segment (fixes the meaning of source and target
	// slices in derived classes)
	Direction _direction;

	// the 2D center of this segment in the inter-section interval
	util::point<double, 2> _center;

	// the number of the inter-section interval this segment lives in
	unsigned int _interSectionInterval;
};

#endif // CELLTRACKER_TRACKLET_H__

