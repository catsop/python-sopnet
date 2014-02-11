#ifndef SLICE_POINTER_HASH_H__
#define SLICE_POINTER_HASH_H__
#include <boost/unordered_set.hpp>
#include <sopnet/segments/Segment.h>

class SlicePointerHash
{
public:
	size_t operator()(const boost::shared_ptr<Slice>& slice) const
	{
		return hash_value(*slice);
	}
};

class SlicePointerEquals
{
public:
	bool operator()(const boost::shared_ptr<Slice>& s1,
					const boost::shared_ptr<Slice>& s2) const
	{
		return (*s1) == (*s2);
	}
};

typedef boost::unordered_set<boost::shared_ptr<Slice>, SlicePointerHash, SlicePointerEquals >
	SliceSet;

#endif //SLICE_POINTER_HASH_H__