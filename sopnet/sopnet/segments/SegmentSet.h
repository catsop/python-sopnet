#ifndef SEGMENT_SET_H__
#define SEGMENT_SET_H__

#include <set>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <sopnet/segments/Segment.h>
#include <sopnet/segments/Segments.h>

/**
 * A slow-but-steady Segments set. boost::unordered_set and std::set don't seem to do the job we
 * want, so we'll roll our own.
 */

class SegmentComparator
{
public:
	bool operator()(const boost::shared_ptr<Segment> segment1,
						   const boost::shared_ptr<Segment> segment2) const
	{
		return (*segment1) < (*segment2);
	}
};

class SegmentSet
{
	typedef std::set<boost::shared_ptr<Segment>, SegmentComparator> segment_set;
public:
	typedef segment_set::iterator iterator;
	typedef segment_set::const_iterator const_iterator;
	
	SegmentSet(){}

	void add(const boost::shared_ptr<Segment> segment);
	
	void addAll(const boost::shared_ptr<Segments> segments);
	
	bool contains(const boost::shared_ptr<Segment> segment) const;
	
	boost::shared_ptr<Segment> find(const boost::shared_ptr<Segment> segment) const;
	
	const const_iterator begin() const { return _segments.begin(); }

	iterator begin() { return _segments.begin(); }

	const const_iterator end() const { return _segments.end(); }

	iterator end() { return _segments.end(); }

	unsigned int size() const { return _segments.size(); }
	
private:
	segment_set _segments;
};


#endif //SEGMENT_SET_H__