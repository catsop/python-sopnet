#ifndef HOST_GRAPH_NODE_H__
#define HOST_GRAPH_NODE_H__

#include "GraphBase.h"

namespace host {

typedef GraphBase::Node            Node;
typedef GraphBase::NodeIt          NodeIt;
typedef GraphBase::NodeMap<double> NodeWeights;
typedef GraphBase::NodeMap<bool>   NodeSelection;

} // namespace host

#endif // HOST_GRAPH_NODE_H__

