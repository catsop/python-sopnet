#include <lemon/lgf_reader.h>
#include <util/exceptions.h>
#include <graph/Logging.h>
#include "WeightedGraphReader.h"

namespace host {

std::istream& operator>>(std::istream& is, host::ArcType& type) {

	unsigned int i;
	is >> i;
	type = static_cast<host::ArcType>(i);

	return is;
}

void
WeightedGraphReader::fill(
		host::Graph& graph,
		host::ArcWeights& weights,
		host::ArcLabels& labels,
		host::ArcTypes& types) {

	std::ifstream is(_filename.c_str());

	bool isUndirected = true;
	int  rootId = 0;

	lemon::digraphReader(graph, is).
			arcMap("weight", weights).
			arcMap("label", labels).
			arcMap("type", types).
			attribute("undirected", isUndirected).
			attribute("root", rootId).
			run();

	graph.setUndirected(isUndirected);
	graph.setRoot(graph.nodeFromId(rootId));

	if (isUndirected) {

		std::vector<Arc> linkArcs;

		// add arcs in opposite direction for each link arc
		for (host::ArcIt arc(graph); arc != lemon::INVALID; ++arc)
			if (types[arc] == host::Link)
				linkArcs.push_back(arc);

		for (const Arc& arc : linkArcs)
			addOppositeArc(graph, arc, weights, labels, types);
	}

	std::vector<Arc> conflictArcs;

	// add arcs in opposite direction for each conflict arc
	for (host::ArcIt arc(graph); arc != lemon::INVALID; ++arc)
		if (types[arc] == host::Conflict)
			conflictArcs.push_back(arc);

	for (const Arc& arc : conflictArcs)
		addOppositeArc(graph, arc, weights, labels, types);
}

void
WeightedGraphReader::addOppositeArc(
		host::Graph& graph,
		const host::Arc& arc,
		host::ArcWeights& weights,
		host::ArcLabels& labels,
		host::ArcTypes& types) {

	Edge edge = graph.edgeFromArc(arc);
	if (edge.size() > 1)
			UTIL_THROW_EXCEPTION(
					UsageError,
					graph << "graph does containt two arcs for " << arc << ", but is marked as undirected");

	host::Arc opposite = graph.addArc(graph.target(arc), graph.source(arc));

	weights[opposite] = weights[arc];
	labels[opposite]  = labels[arc] + std::string("_opp");
	types[opposite]   = types[arc];
}

} // namespace host
