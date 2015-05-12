#include <util/Logger.h>
#include <util/exceptions.h>
#include <graph/Logging.h>
#include "Configuration.h"
#include "CandidateConflictTerm.h"

namespace host {

logger::LogChannel cctlog("cctlog", "[CandidateConflictTerm] ");

CandidateConflictTerm::CandidateConflictTerm(
		const Graph& graph,
		const ArcTypes& arcTypes) :
	_graph(graph) {

	findExclusiveEdges(arcTypes);
	findConflictArcs(arcTypes);
}

size_t
CandidateConflictTerm::numLambdas() const {

	size_t num= 0;

	for (const auto& term : _exclusiveEdges)
		num += term.numLambdas();
	for (const auto& term : _exclusiveArcs)
		num += term.numLambdas();

	return num;
}

void
CandidateConflictTerm::lambdaBounds(
		Lambdas::iterator beginLower,
		Lambdas::iterator endLower,
		Lambdas::iterator beginUpper,
		Lambdas::iterator endUpper) {

	for (auto& exclusive : _exclusiveEdges) {

		exclusive.lambdaBounds(
				beginLower,
				beginLower + exclusive.numLambdas(),
				beginUpper,
				beginUpper + exclusive.numLambdas());

		beginLower += exclusive.numLambdas();
		beginUpper += exclusive.numLambdas();
	}

	for (auto& exclusive : _exclusiveArcs) {

		exclusive.lambdaBounds(
				beginLower,
				beginLower + exclusive.numLambdas(),
				beginUpper,
				beginUpper + exclusive.numLambdas());

		beginLower += exclusive.numLambdas();
		beginUpper += exclusive.numLambdas();
	}

	if (beginUpper != endUpper || beginLower != endLower)
		UTIL_THROW_EXCEPTION(
				UsageError,
				"given range of lambdas does not match number of lambdas");
}

void
CandidateConflictTerm::setLambdas(Lambdas::const_iterator begin, Lambdas::const_iterator end) {

	Lambdas::const_iterator i = begin;

	LOG_ALL(cctlog) << "λ set to :" << std::endl;

	for (auto& exclusive : _exclusiveEdges) {

		exclusive.setLambdas(i, i + exclusive.numLambdas());
		i += exclusive.numLambdas();

		LOG_ALL(cctlog) << exclusive << std::endl;
	}

	for (auto& exclusive : _exclusiveArcs) {

		exclusive.setLambdas(i, i + exclusive.numLambdas());
		i += exclusive.numLambdas();

		LOG_ALL(cctlog) << exclusive << std::endl;
	}

	LOG_ALL(cctlog) << std::endl;

	if (i != end)
		UTIL_THROW_EXCEPTION(
				UsageError,
				"given range of lambdas does not match number of lambdas");
}

void
CandidateConflictTerm::addArcWeights(ArcWeights& weights) {

	for (auto& exclusive : _exclusiveEdges)
		exclusive.addArcWeights(weights);

	for (auto& exclusive : _exclusiveArcs)
		exclusive.addArcWeights(weights);
}

double
CandidateConflictTerm::constant() {

	double constant = 0;

	for (auto exclusive : _exclusiveEdges)
		constant += exclusive.constant();

	for (auto exclusive : _exclusiveArcs)
		constant += exclusive.constant();

	return constant;
}

bool
CandidateConflictTerm::gradient(
		const ArcSelection& mst,
		Lambdas::iterator   begin,
		Lambdas::iterator   end) {

	Lambdas::iterator i = begin;

	bool feasible = true;

	LOG_ALL(cctlog) << "gradient is:" << std::endl;

	for (auto& exclusive : _exclusiveEdges) {

		feasible &= exclusive.gradient(mst, i, i + exclusive.numLambdas());

		LOG_ALL(cctlog) << exclusive;
		for (Lambdas::iterator j = i; j != i + exclusive.numLambdas(); j++)
			LOG_ALL(cctlog) << " " << *j;
		LOG_ALL(cctlog) << std::endl;

		i += exclusive.numLambdas();
	}

	for (auto& exclusive : _exclusiveArcs) {

		feasible &= exclusive.gradient(mst, i, i + exclusive.numLambdas());

		LOG_ALL(cctlog) << exclusive;
		for (Lambdas::iterator j = i; j != i + exclusive.numLambdas(); j++)
			LOG_ALL(cctlog) << " " << *j;
		LOG_ALL(cctlog) << std::endl;

		i += exclusive.numLambdas();
	}

	LOG_ALL(cctlog) << std::endl;

	if (i != end)
		UTIL_THROW_EXCEPTION(
				UsageError,
				"given range of lambdas does not match number of lambdas");

	return feasible;
}

void
CandidateConflictTerm::findExclusiveEdges(const ArcTypes& arcTypes) {

	// for each conflict edge
	for (Graph::ArcIt arc(_graph); arc != lemon::INVALID; ++arc) {

		if (arcTypes[arc] != Conflict)
			continue;

		// consider only one direction
		if (_graph.id(_graph.source(arc)) >= _graph.id(_graph.target(arc)))
			continue;

		Edges sourceEdges = findLinkEdges(_graph.source(arc), arcTypes);
		Edges targetEdges = findLinkEdges(_graph.target(arc), arcTypes);

		// for each source link edge
		for (const Edge& sourceEdge : sourceEdges) {

			// for each target link edge
			for (const Edge& targetEdge : targetEdges) {

				if (sourceEdge == targetEdge)
					UTIL_THROW_EXCEPTION(
							UsageError,
							"conflict arc (" << _graph.id(_graph.source(arc)) << ", " << _graph.id(_graph.target(arc)) <<
							" has parallel link arcs: " << _graph << sourceEdge);

				ExclusiveEdgesTerm term(sourceEdge,targetEdge);

				if (std::find(_exclusiveEdges.begin(), _exclusiveEdges.end(), term) == _exclusiveEdges.end())
					_exclusiveEdges.push_back(term);
			}
		}
	}

	LOG_ALL(cctlog)
			<< "exclusive edges are:" << std::endl;
	for (const auto& exclusive : _exclusiveEdges)
		LOG_ALL(cctlog)
				<< "\t" << _graph << exclusive << std::endl;
	LOG_ALL(cctlog) << std::endl;
}

CandidateConflictTerm::Edges
CandidateConflictTerm::findLinkEdges(const Node& node, const ArcTypes& arcTypes) {

	Edges edges;
	std::map<Node, size_t> nodeEdgeMap;

	// for each outgoing link arc
	for (OutArcIt out(_graph, node); out != lemon::INVALID; ++out) {

		if (arcTypes[out] != Link)
			continue;

		// create one edge
		Edge edge;
		edge.addArc(out);
		edges.push_back(edge);

		// remember mapping of target node to edge
		nodeEdgeMap[_graph.target(out)] = edges.size() - 1;
	}

	// for each incoming link arc
	for (InArcIt in(_graph, node); in != lemon::INVALID; ++in) {

		if (arcTypes[in] != Link)
			continue;

		// add to existing edge...
		if (nodeEdgeMap.count(_graph.source(in))) {

			edges[nodeEdgeMap[_graph.source(in)]].addArc(in);

		// ...or create a new one
		} else {

			Edge edge;
			edge.addArc(in);
			edges.push_back(edge);
		}
	}

	return edges;
}

void
CandidateConflictTerm::findConflictArcs(const ArcTypes& arcTypes) {

	// for each conflict arc
	for (Graph::ArcIt arc(_graph); arc != lemon::INVALID; ++arc) {

		if (arcTypes[arc] != Conflict)
			continue;

		// for each outgoing conflict arc
		for (OutArcIt out(_graph, _graph.target(arc)); out != lemon::INVALID; ++out)
			if (arcTypes[out] == Conflict && _graph.target(out) != _graph.source(arc))
				_exclusiveArcs.push_back(ExclusiveArcsTerm(arc, out));
	}

	LOG_ALL(cctlog)
			<< "conflict arcs are:" << std::endl;
	for (const auto& exclusive : _exclusiveArcs)
		LOG_ALL(cctlog)
				<< "\t" << _graph << exclusive
				<< std::endl;
	LOG_ALL(cctlog) << std::endl;
}

} // namespace host
