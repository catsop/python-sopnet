#include "Segment.h"

#include <util/Logger.h>

logger::LogChannel segmentlog("segmentlog", "[Segment] ");

Segment::Segment(
		unsigned int id,
		Direction direction,
		const util::point<double>& center,
		unsigned int interSectionInterval) :
	_id(id),
	_direction(direction),
	_center(center),
	_interSectionInterval(interSectionInterval) {}

unsigned int
Segment::getNextSegmentId() {

	unsigned int id;

	{
		boost::mutex::scoped_lock lock(SegmentIdMutex);

		id = NextSegmentId;

		NextSegmentId++;
	}

	return id;
}

unsigned int
Segment::getId() const {

	return _id;
}

Direction
Segment::getDirection() const {

	return _direction;
}

unsigned int
Segment::getInterSectionInterval() const {

	return _interSectionInterval;
}

std::vector<boost::shared_ptr<Slice> >
Segment::getSourceSlices() const {

	std::vector<boost::shared_ptr<Slice> > slices = getSlices();

	unsigned int interSectionInterval = getInterSectionInterval();
	unsigned int sourceSection = (getDirection() == Left ? interSectionInterval : interSectionInterval - 1);

	std::vector<boost::shared_ptr<Slice> > sourceSlices;

	foreach (boost::shared_ptr<Slice> slice, slices)
		if (slice->getSection() == sourceSection)
			sourceSlices.push_back(slice);

	return sourceSlices;
}

std::vector<boost::shared_ptr<Slice> >
Segment::getTargetSlices() const {

	std::vector<boost::shared_ptr<Slice> > slices = getSlices();

	unsigned int interSectionInterval = getInterSectionInterval();
	unsigned int targetSection = (getDirection() == Left ? interSectionInterval - 1 : interSectionInterval);

	std::vector<boost::shared_ptr<Slice> > targetSlices;

	foreach (boost::shared_ptr<Slice> slice, slices)
		if (slice->getSection() == targetSection)
			targetSlices.push_back(slice);

	return targetSlices;
}

bool
Segment::operator==(const Segment& other) const
{
	if (getDirection() == other.getDirection() &&
		getSlices().size() == other.getSlices().size())
	{
		// Worst-case O(N * N), but N is at most 3.
		// Typical-case O(N)
		
		foreach (boost::shared_ptr<Slice> slice, getSlices())
		{
			bool contains = false;
			foreach (boost::shared_ptr<Slice> otherSlice, other.getSlices())
			{
				if (*otherSlice == *slice)
				{
					contains = true;
					break;
				}
			}
			
			if (!contains)
			{
				return false;
			}
		}
		
		return true;
	}
	else
	{
		return false;
	}
}

std::string Segment::typeString(const SegmentType type)
{
	switch (type)
	{
		case EndSegmentType:
			return "end";
		case ContinuationSegmentType:
			return "continuation";
		case BranchSegmentType:
			return "branch";
		default:
			return "unknown";
	}
}

unsigned int Segment::NextSegmentId = 0;
boost::mutex Segment::SegmentIdMutex;
