#ifndef HOST_INFERENCE_EXCLUSIVE_EDGES_TERM_H__
#define HOST_INFERENCE_EXCLUSIVE_EDGES_TERM_H__

#include <ostream>
#include "detail/ExclusiveTermImpl.h"

namespace host {

/**
 * A term to prevent the exclusive selection of two edges.
 */
typedef detail::ExclusiveTermImpl<Edge> ExclusiveEdgesTerm;

} // namespace host

#endif // HOST_INFERENCE_EXCLUSIVE_EDGES_TERM_H__

