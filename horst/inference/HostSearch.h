#ifndef HOST_INFERENCE_HOST_SEARCH_H__
#define HOST_INFERENCE_HOST_SEARCH_H__

#include <vector>
#include "ArcTerm.h"
#include "HigherOrderArcTerm.h"
#include "ProximalBundleMethod.h"

namespace host {

class HostSearch {

public:

	HostSearch(const host::Graph& graph) :
		_currentWeights(graph),
		_graph(graph) {}

	/**
	 * Add an arc term to the objective of this search.
	 */
	void addTerm(ArcTerm* term);

	/**
	 * Find a minimal spanning tree on a consistent subset of the provided 
	 * candidate nodes.
	 *
	 * @param weights
	 *              Arc weights for the graph associated to this search.
	 *
	 * @param mst
	 *              The arcs that are part of the minimal spanning tree.
	 *
	 * @param value
	 *              The length of the minimal spanning tree.
	 *
	 * @param maxIterations
	 *              The maximal number of iterations to spent on the search.
	 *
	 * @return
	 *              True, if a minimal spanning tree that fulfills all 
	 *              constraints could be found.
	 */
	bool find(
			host::ArcSelection& mst,
			double&             value,
			unsigned int        maxIterations = 1000,
			const Lambdas&      initialLambdas = Lambdas());

private:

	class ValueGradientCallback;
	typedef ProximalBundleMethod<ValueGradientCallback> Optimizer;

	class ValueGradientCallback {

	public:

		ValueGradientCallback(
				HostSearch&              hostSearch,
				host::ArcSelection&     mst) :
			_hostSearch(hostSearch),
			_mst(mst) {}

		Optimizer::CallbackResponse operator()(
				const Lambdas& x,
				double&        value,
				Lambdas&       gradient);

	private:

		HostSearch&          _hostSearch;
		host::ArcSelection& _mst;
	};

	size_t numLambdas();

	// set the lambdas in all higher-order arc terms
	void setLambdas(const Lambdas& x);

	// assemble the arc weights from all arc terms
	void updateWeights();

	// find the minimal spanning tree on the current weights
	double mst(host::ArcSelection& currentMst);

	// get the gradient for the given mst
	bool gradient(
			const host::ArcSelection& mst,
			Lambdas&                  gradient);

	// fix the dual value of feasible solutions with non-zero gradients
	double valueFromFeasibleSolution(
			double                    dualValue,
			const Lambdas&            lambdas,
			const Lambdas&            gradient);

	std::vector<ArcTerm*>            _arcTerms;
	std::vector<HigherOrderArcTerm*> _higherOrderArcTerms;

	// the current weights under consideration of all terms
	host::ArcWeights _currentWeights;

	const host::Graph& _graph;

	bool _feasibleSolutionFound;
};

} // namespace host

#endif // HOST_INFERENCE_HOST_SEARCH_H__

