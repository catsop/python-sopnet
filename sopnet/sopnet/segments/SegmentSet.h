#ifndef SEGMENT_SET_H__
#define SEGMENT_SET_H__

#include <vector>
#include <boost/shared_ptr.hpp>
#include <sopnet/segments/Segment.h>

/**
 * A slow-but-steady Segments set. boost::unordered_set and std::set don't seem to do the job we
 * want, so we'll roll our own.
 */

class SegmentSet
{
	typedef std::vector<boost::shared_ptr<Segment> >::iterator iterator;
	typedef std::vector<boost::shared_ptr<Segment> >::const_iterator const_iterator;
	
public:
	SegmentSet(){}

	void add(const boost::shared_ptr<Segment> segment);
	
	bool contains(const boost::shared_ptr<Segment> segment) const;
	
	boost::shared_ptr<Segment> find(const boost::shared_ptr<Segment> segment);
	
	const const_iterator begin() const { return _segments.begin(); }

	iterator begin() { return _segments.begin(); }

	const const_iterator end() const { return _segments.end(); }

	iterator end() { return _segments.end(); }

	unsigned int size() const { return _segments.size(); }
	
private:
	std::vector<boost::shared_ptr<Segment> > _segments;
};


#endif //SEGMENT_SET_H__