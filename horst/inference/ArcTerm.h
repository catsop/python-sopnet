#ifndef HOST_INFERENCE_EDGE_TERM_H__
#define HOST_INFERENCE_EDGE_TERM_H__

#include <graph/Graph.h>

class ArcTerm {

public:

	/**
	 * Add the weights contributed by this arc term to the given arc weights.
	 */
	virtual void addArcWeights(host::ArcWeights& weights) = 0;
};

#endif // HOST_INFERENCE_EDGE_TERM_H__

