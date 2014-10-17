#ifndef SOPNET_CATMAID_PERSISTENCE_SEGMENT_CONSTRAINTS_H__
#define SOPNET_CATMAID_PERSISTENCE_SEGMENT_CONSTRAINTS_H__

#include <vector>
#include <segments/SegmentHash.h>

typedef std::set<SegmentHash> SegmentConstraint;
typedef std::vector<SegmentConstraint> SegmentConstraints;

#endif // SOPNET_CATMAID_PERSISTENCE_SEGMENT_CONSTRAINTS_H__