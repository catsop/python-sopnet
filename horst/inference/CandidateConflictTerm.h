#ifndef HOST_CANDIDATE_CONFLICT_TERM_H__
#define HOST_CANDIDATE_CONFLICT_TERM_H__

#include <graph/Graph.h>
#include "HigherOrderArcTerm.h"
#include "ExclusiveEdgesTerm.h"
#include "ExclusiveArcsTerm.h"

namespace host {

class CandidateConflictTerm : public HigherOrderArcTerm {

public:

	/**
	 * Construct a candidate conflict term for the given graph.
	 */
	CandidateConflictTerm(const Graph& graph, const ArcTypes& arcTypes);

	/**
	 * Get the number of lambda parameters of this higher order term.
	 */
	size_t numLambdas() const;

	/**
	 * Store the upper and lower bounds for each lambda in the the given ranges.
	 */
	void lambdaBounds(
			Lambdas::iterator beginLower,
			Lambdas::iterator endLower,
			Lambdas::iterator beginUpper,
			Lambdas::iterator endUpper);

	/**
	 * Set the lambda parameters.
	 */
	void setLambdas(Lambdas::const_iterator begin, Lambdas::const_iterator end);

	/**
	 * Add the lambda contributions of this higher order term to the given arc 
	 * weights.
	 */
	void addArcWeights(ArcWeights& weights);

	/**
	 * Get the constant contribution of this higher order term to the objective.
	 */
	double constant();

	/**
	 * For the given MST (represented as boolean flags on arcs), compute the 
	 * gradient for each lambda and store it in the range pointed to with the 
	 * given iterator.
	 *
	 * @return true, if the current mst is feasible
	 */
	bool gradient(
			const ArcSelection& mst,
			Lambdas::iterator   begin,
			Lambdas::iterator   end);

private:

	typedef std::vector<Edge> Edges;

	// find pairs of mutual exclusive arcs
	void findExclusiveEdges(const ArcTypes& arcTypes);

	// find all link edges of a node
	Edges findLinkEdges(const Node& node, const ArcTypes& arcTypes);

	// find conflict nodes and add an ConflictArcsLambda for each incoming edge
	void findConflictArcs(const ArcTypes& arcTypes);

	// list of containing terms
	std::vector<ExclusiveEdgesTerm> _exclusiveEdges;
	std::vector<ExclusiveArcsTerm>  _exclusiveArcs;

	const Graph& _graph;
};

} // namespace host

#endif // HOST_CANDIDATE_CONFLICT_TERM_H__

