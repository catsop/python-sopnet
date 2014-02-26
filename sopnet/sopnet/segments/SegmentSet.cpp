#include "SegmentSet.h"

void
SegmentSet::add(const boost::shared_ptr<Segment> segment)
{
	if (!contains(segment))
	{
		_segments.push_back(segment);
	}
}

bool
SegmentSet::contains(const boost::shared_ptr<Segment> segment) const
{
	foreach (const boost::shared_ptr<Segment> hereSegment, _segments)
	{
		if (*segment == *hereSegment)
		{
			return true;
		}
	}
	
	return false;
}

boost::shared_ptr<Segment>
SegmentSet::find(const boost::shared_ptr<Segment> segment)
{
	foreach (boost::shared_ptr<Segment> hereSegment, _segments)
	{
		if (*segment == *hereSegment)
		{
			return hereSegment;
		}
	}
	
	return boost::shared_ptr<Segment>();
}

