#include "WeightedGraphGenerator.h"

class RandomWeightedGraphGenerator {

public:

	RandomWeightedGraphGenerator(
			unsigned int numVertices,
			unsigned int numArcs,
			double minArcWeight,
			double maxArcWeight) :
		_numVertices(numVertices),
		_numArcs(numArcs),
		_minArcWeight(minArcWeight),
		_maxArcWeight(maxArcWeight) {}

	void fill(
			host::Graph& graph,
			host::ArcWeights& weights,
			host::ArcLabels& labels,
			host::ArcTypes& types);

private:

	unsigned int _numVertices;
	unsigned int _numArcs;
	double _minArcWeight;
	double _maxArcWeight;
};
