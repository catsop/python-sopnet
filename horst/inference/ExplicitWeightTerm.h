#ifndef HOST_INFERENCE_INITIAL_WEIGHT_TERM_H__
#define HOST_INFERENCE_INITIAL_WEIGHT_TERM_H__

#include "ArcTerm.h"

namespace host {

/**
 * An arc term contributing an initial set of arc weights.
 */
class ExplicitWeightTerm : public ArcTerm {

public:

	ExplicitWeightTerm(
			const Graph& graph,
			const ArcWeights& weights) :
		_graph(graph),
		_weights(weights) {}

	void addArcWeights(ArcWeights& weights) {

		for (Graph::ArcIt arc(_graph); arc != lemon::INVALID; ++arc)
			weights[arc] += _weights[arc];
	}

private:

	const Graph& _graph;

	const ArcWeights& _weights;
};

} // namespace host

#endif // HOST_INFERENCE_INITIAL_WEIGHT_TERM_H__

