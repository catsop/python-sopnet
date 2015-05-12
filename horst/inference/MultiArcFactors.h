#ifndef HOST_INFERENCE_MULTI_ARC_FACTORS_H__
#define HOST_INFERENCE_MULTI_ARC_FACTORS_H__

#include <map>
#include <vector>
#include <graph/Graph.h>
#include "detail/MultiFactorTermImpl.h"

namespace host {

typedef detail::MultiFactorsImpl<Arc> MultiArcFactors;

} // namespace host

#endif // HOST_INFERENCE_MULTI_ARC_FACTORS_H__

