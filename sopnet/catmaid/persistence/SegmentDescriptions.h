#ifndef SOPNET_CATMAID_PERSISTENCE_SEGMENT_DESCRIPTIONS_H__
#define SOPNET_CATMAID_PERSISTENCE_SEGMENT_DESCRIPTIONS_H__

#include <vector>
#include "SegmentDescription.h"

class SegmentDescriptions {

public:

	typedef std::vector<SegmentDescription> segments_type;
	typedef segments_type::iterator         iterator;
	typedef segments_type::const_iterator   const_iterator;

	void add(const SegmentDescription& segment) { _segments.push_back(segment); }

	iterator begin() { return _segments.begin(); }
	iterator end() { return _segments.end(); }
	const_iterator begin() const { return _segments.begin(); }
	const_iterator end() const { return _segments.end(); }

private:

	segments_type _segments;
};

#endif // SOPNET_CATMAID_PERSISTENCE_SEGMENT_DESCRIPTION_H__

