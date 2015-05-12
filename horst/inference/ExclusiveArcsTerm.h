#ifndef HOST_INFERENCE_EXCLUSIVE_ARCS_TERM_H__
#define HOST_INFERENCE_EXCLUSIVE_ARCS_TERM_H__

#include <ostream>
#include "HigherOrderArcTerm.h"

namespace host {

/**
 * A term to prevent the exclusive selection of two arcs.
 */
typedef detail::ExclusiveTermImpl<Arc> ExclusiveArcsTerm;

} // namespace host


#endif // HOST_INFERENCE_EXCLUSIVE_ARCS_TERM_H__

