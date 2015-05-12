#include <util/Logger.h>
#include "RandomWeightedGraphGenerator.h"

logger::LogChannel randomwggeneratorlog("randomwggeneratorlog", "[RandomWeightedGraphGenerator] ");

void
RandomWeightedGraphGenerator::fill(
		host::Graph& graph,
		host::ArcWeights& weights,
		host::ArcLabels& /*labels*/,
		host::ArcTypes& types) {

	std::vector<host::Graph::Node> vertices;

	srand(23);

	for (unsigned int i = 0; i < _numVertices; i++)
		vertices.push_back(graph.addNode());

	for (unsigned int i = 0; i < _numArcs; i++) {

		host::Graph::Node u;
		host::Graph::Node v;

		for (;;) {

			bool validArc = true;

			u = vertices[rand()%vertices.size()];
			v = vertices[rand()%vertices.size()];

			if (u == v)
				continue;

			for (host::OutArcIt arc(graph, u); arc != lemon::INVALID; ++arc) {

				if (graph.source(arc) == v || graph.target(arc) == v) {

					validArc = false;
					break;
				}
			}

			if (validArc)
				break;
		}

		LOG_ALL(randomwggeneratorlog)
				<< "adding arc " << graph.id(u) << " - " << graph.id(v) << std::endl;

		double weight = _minArcWeight + (static_cast<double>(rand())/RAND_MAX)*(_maxArcWeight - _minArcWeight);

		host::Graph::Arc e = graph.addArc(u, v);
		weights[e] = weight;
		types[e] = host::Link;
	}
}
