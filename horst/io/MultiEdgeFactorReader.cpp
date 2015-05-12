#include <fstream>
#include "MultiEdgeFactorReader.h"

namespace host {

void
MultiEdgeFactorReader::fill(
		const Graph&      graph,
		const ArcLabels&  labels,
		MultiEdgeFactors& factors) {

	std::ifstream in(_filename);

	// create a reverse look-up from labels to arcs
	std::map<std::string, Arc> labelEdgeMap;
	for (host::ArcIt arc(graph); arc != lemon::INVALID; ++arc)
		labelEdgeMap[labels[arc]] = arc;

	std::string line;
	while (std::getline(in, line)) {

		double value;
		MultiEdgeFactors::Edges edges;

		std::stringstream ss(line);

		ss >> value;
		while (ss.good()) {

			std::string label;
			ss >> label;
			edges.push_back(graph.edgeFromArc(labelEdgeMap[label]));
		}

		factors[edges] = value;
	}
}

} // namespace host
