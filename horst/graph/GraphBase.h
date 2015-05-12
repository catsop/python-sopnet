#ifndef HOST_GRAPH_GRAPH_BASE_H__
#define HOST_GRAPH_GRAPH_BASE_H__

namespace host {

typedef lemon::ListDigraph GraphBase;

typedef GraphBase::Arc                 Arc;
typedef GraphBase::ArcIt               ArcIt;
typedef GraphBase::OutArcIt            OutArcIt;
typedef GraphBase::InArcIt             InArcIt;

} // namespace host

#endif // HOST_GRAPH_GRAPH_BASE_H__

