#ifndef SEGMENT_POINTER_HASH_H__
#define SEGMENT_POINTER_HASH_H__
#include <boost/unordered_set.hpp>
#include <sopnet/segments/Segment.h>
#include <sopnet/segments/SegmentHash.h>

class SegmentPointerHash
{
public:
	size_t operator()(const boost::shared_ptr<Segment>& segment) const
	{
		return hash_value(*segment);
	}
};

class SegmentPointerEquals
{
public:
	bool operator()(const boost::shared_ptr<Segment>& s1,
					const boost::shared_ptr<Segment>& s2) const
	{
		return (*s1) == (*s2);
	}
};

typedef boost::unordered_set<boost::shared_ptr<Segment>, SegmentPointerHash, SegmentPointerEquals >
	SegmentSetType;

#endif //SEGMENT_POINTER_HASH_H__
