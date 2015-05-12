#include <lemon/min_cost_arborescence.h>
#include <util/Logger.h>
#include <graph/Logging.h>
#include "HostSearch.h"

logger::LogChannel hostsearchlog("hostsearchlog", "[HostSearch] ");

namespace host {

void
HostSearch::addTerm(ArcTerm* term) {

	_arcTerms.push_back(term);

	if (HigherOrderArcTerm* higherOrderTerm = dynamic_cast<HigherOrderArcTerm*>(term))
		_higherOrderArcTerms.push_back(higherOrderTerm);
}

bool
HostSearch::find(
		host::ArcSelection& mst,
		double&             value,
		unsigned int        maxIterations,
		const Lambdas&      initialLambdas) {

	ValueGradientCallback valueGradientCallback(*this, mst);

	Optimizer optimizer(
			numLambdas(),
			maxIterations,
			valueGradientCallback);

	Lambdas lowerBounds(numLambdas(), -Optimizer::Infinity);
	Lambdas upperBounds(numLambdas(),  Optimizer::Infinity);
	Lambdas::iterator li = lowerBounds.begin();
	Lambdas::iterator ui = upperBounds.begin();

	for (auto* term : _higherOrderArcTerms) {

		term->lambdaBounds(li, li + term->numLambdas(), ui, ui + term->numLambdas());

		li += term->numLambdas();
		ui += term->numLambdas();
	}

	for (unsigned int lambdaNum = 0; lambdaNum < numLambdas(); lambdaNum++)
		optimizer.setVariableBound(lambdaNum, lowerBounds[lambdaNum], upperBounds[lambdaNum]);

	if (initialLambdas.size() > 0)
		optimizer.setInitialPosition(initialLambdas.begin(), initialLambdas.end());

	optimizer.optimize();

	LOG_ALL(hostsearchlog)
			<< "final weights are:" << _graph << std::endl;
	for (host::Graph::ArcIt arc(_graph); arc != lemon::INVALID; ++arc)
		LOG_ALL(hostsearchlog) << (Arc)arc << ": " << _currentWeights[arc] << std::endl;

	LOG_DEBUG(hostsearchlog)
			<< "mst is:" << _graph << std::endl;
	for (host::Graph::ArcIt arc(_graph); arc != lemon::INVALID; ++arc)
		LOG_DEBUG(hostsearchlog) << arc << ": " << mst[arc] << std::endl;

	if (optimizer.getStatus() == Optimizer::Stopped && _feasibleSolutionFound) {

		value = valueFromFeasibleSolution(
				optimizer.getOptimalValue(),
				optimizer.getOptimalPosition(),
				optimizer.getOptimalGradient());

		return false;
	}

	value = optimizer.getOptimalValue();

	LOG_DEBUG(hostsearchlog)
			<< "length of mst is " << value << std::endl;

	if (optimizer.getStatus() == Optimizer::ExactOptimiumFound)
		return true;

	return false;
}

HostSearch::Optimizer::CallbackResponse
HostSearch::ValueGradientCallback::operator()(
		const Lambdas& lambdas,
		double&        value,
		Lambdas&       gradient) {

	_hostSearch.setLambdas(lambdas);

	_hostSearch.updateWeights();

	value = _hostSearch.mst(_mst);

	bool feasible = _hostSearch.gradient(_mst, gradient);

	LOG_ALL(hostsearchlog) << "current value of dual is " << value << std::endl;

	if (feasible)
		LOG_USER(hostsearchlog) << "Feasible solution found." << std::endl;

	return Optimizer::Continue;
}

size_t
HostSearch::numLambdas() {

	size_t numLambdas = 0;

	for (auto* term : _higherOrderArcTerms)
		numLambdas += term->numLambdas();

	return numLambdas;
}

void
HostSearch::setLambdas(const Lambdas& x) {

	Lambdas::const_iterator i = x.begin();

	for (auto* term : _higherOrderArcTerms) {

		term->setLambdas(i, i + term->numLambdas());
		i += term->numLambdas();
	}
}

void
HostSearch::updateWeights() {

	// set weights to zero
	for (host::Graph::ArcIt arc(_graph); arc != lemon::INVALID; ++arc)
		_currentWeights[arc] = 0;

	for (auto* term : _arcTerms)
		term->addArcWeights(_currentWeights);

	LOG_ALL(hostsearchlog) << "updated weights are:" << std::endl;
	for (ArcIt arc(_graph); arc != lemon::INVALID; ++arc)
		LOG_ALL(hostsearchlog)
				<< "\t" << _graph << arc
				<< "\t" << _currentWeights[arc] << std::endl;
	LOG_ALL(hostsearchlog) << std::endl;
}

double
HostSearch::mst(host::ArcSelection& currentMst) {

	double mstValue = lemon::minCostArborescence(_graph, _currentWeights, _graph.getRoot(), currentMst);

	LOG_ALL(hostsearchlog)
			<< "minimal spanning tree with root at "
			<< _graph.id(_graph.getRoot())
			<< " is:" << std::endl;
	for (host::Graph::ArcIt arc(_graph); arc != lemon::INVALID; ++arc)
		LOG_ALL(hostsearchlog)
				<< _graph.id(_graph.source(arc)) << " - "
				<< _graph.id(_graph.target(arc)) << ": "
				<< currentMst[arc] << std::endl;
	LOG_ALL(hostsearchlog) << std::endl;

	// to the mst value obtained above, we have to add a constant for each 
	// higher order term
	for (auto* term : _higherOrderArcTerms)
		mstValue += term->constant();

	return mstValue;
}

bool
HostSearch::gradient(
			const host::ArcSelection& mst,
			Lambdas&                   gradient) {

	Lambdas::iterator i = gradient.begin();

	_feasibleSolutionFound = true;

	for (auto* term : _higherOrderArcTerms) {

		_feasibleSolutionFound &= term->gradient(mst, i, i + term->numLambdas());
		i += term->numLambdas();
	}

	return _feasibleSolutionFound;
}

double
HostSearch::valueFromFeasibleSolution(
		double                    dualValue,
		const Lambdas&            lambdas,
		const Lambdas&            gradient) {

	Lambdas::const_iterator l = lambdas.begin();
	Lambdas::const_iterator g = gradient.begin();

	double offset = 0;
	for (; l != lambdas.end(); l++, g++)
		offset += (*l)*(*g);

	return dualValue - offset;
}

} // namespace host
