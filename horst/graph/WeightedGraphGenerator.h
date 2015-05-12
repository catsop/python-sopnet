#ifndef HOST_WEIGHTED_GRAPH_GENERATOR_H__
#define HOST_WEIGHTED_GRAPH_GENERATOR_H__

#include "Graph.h"

class WeightedGraphGenerator {

public:

	/**
	 * Create a weighted graph.
	 */
	virtual void fill(
			host::Graph& graph,
			host::ArcWeights& weights,
			host::ArcLabels& labels,
			host::ArcTypes& types) = 0;
};

#endif // HOST_GRAPH_GENERATOR_H__

