#ifndef HOST_INFERENCE_DETAIL_MULTI_FACTOR_TERM_IMPL_H__
#define HOST_INFERENCE_DETAIL_MULTI_FACTOR_TERM_IMPL_H__

#include <map>
#include <inference/HigherOrderArcTerm.h>
#include <inference/Configuration.h>
#include <graph/Logging.h>
#include <util/Logger.h>
#include <util/exceptions.h>
#include "MultiFactorsImpl.h"

extern logger::LogChannel meflog;

namespace host {
namespace detail {

template <typename EdgeType>
class MultiFactorTermImpl : public HigherOrderArcTerm {

public:

	MultiFactorTermImpl<EdgeType>(
			const host::Graph&                graph,
			const MultiFactorsImpl<EdgeType>& factors) :
		_graph(graph),
		_factors(factors) {}

	/**
	 * Get the number of lambda parameters of this higher order term.
	 */
	size_t numLambdas() const;

	/**
	 * Change the upper and lower bounds for each lambda in the the given 
	 * ranges. The defaults are -inf for the lower bounds and +inf for the upper 
	 * bounds.
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
	void addArcWeights(host::ArcWeights& weights);

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
			const host::ArcSelection& mst,
			Lambdas::iterator         begin,
			Lambdas::iterator         end);

private:

	typedef typename MultiFactorsImpl<EdgeType>::Edges EdgesType;

	const host::Graph& _graph;

	MultiFactorsImpl<EdgeType>                   _factors;
	std::map<EdgesType,std::pair<double,double>> _lambdas;

	// indicators for joint arc selection (one per factor)
	std::map<EdgesType,bool> _z;

	// the constant contribution of this term
	double _constant;
};

////////////////////
// IMPLEMENTATION //
////////////////////

template <typename EdgeType>
size_t
MultiFactorTermImpl<EdgeType>::numLambdas() const {

	// two lambdas per factor
	return 2*_factors.size();
}

template <typename EdgeType>
void
MultiFactorTermImpl<EdgeType>::lambdaBounds(
		Lambdas::iterator beginLower,
		Lambdas::iterator endLower,
		Lambdas::iterator /*beginUpper*/,
		Lambdas::iterator /*endUpper*/) {

	// all our lambdas should be positive
	for (Lambdas::iterator i = beginLower; i != endLower; i++)
		*i = 0;
}

template <typename EdgeType>
void
MultiFactorTermImpl<EdgeType>::setLambdas(Lambdas::const_iterator begin, Lambdas::const_iterator end) {

	Lambdas::const_iterator i = begin;

	_constant = 0;

	LOG_ALL(meflog) << "lambdas set to:" << std::endl;

	for (const auto& factor : _factors) {

		const EdgesType& edges = factor.first;

		double lambda1 = *i; i++;
		double lambda2 = *i; i++;

		// set the lambdas

		_lambdas[edges] = std::make_pair(lambda1, lambda2);

		// update z on the fly

		const double value = factor.second;

		// the cost w_f for this z_f
		double w = value + 2*lambda1 - lambda2;

		// z_f == 1 iff w_f < 0
		bool z = (w < 0);
		_z[edges] = z;

		_constant += z*w - lambda2;

		LOG_ALL(meflog)
				<< "\t" << _graph << edges
				<< ": λ¹ = " << lambda1
				<< ",\tλ² = " << lambda2
				<< ",\tz = " << z
				<< std::endl;
	}

	LOG_ALL(meflog) << std::endl;

	if (i != end)
		UTIL_THROW_EXCEPTION(
				UsageError,
				"given range of lambdas does not match number of lambdas");
}

template <typename EdgeType>
void
MultiFactorTermImpl<EdgeType>::addArcWeights(host::ArcWeights& weights) {

	for (const auto& edgesLambdas : _lambdas) {

		const EdgesType&                 edges   = edgesLambdas.first;
		const std::pair<double, double>& lambdas = edgesLambdas.second;

		for (auto& edge : edges) {

			// lambda1 gets subtracted from the arc weights
			weights[edge] -= lambdas.first;

			// lambda2 gets added to the arc weights
			weights[edge] += lambdas.second;
		}
	}
}

template <typename EdgeType>
double
MultiFactorTermImpl<EdgeType>::constant() {

	return _constant;
}

template <typename EdgeType>
bool
MultiFactorTermImpl<EdgeType>::gradient(
		const host::ArcSelection& mst,
		Lambdas::iterator         begin,
		Lambdas::iterator         /*end*/) {

	Lambdas::iterator i = begin;

	bool feasible = true;

	LOG_ALL(meflog) << "gradient is:" << std::endl;

	for (const auto& factor : _factors) {

		const EdgesType& edges                   = factor.first;
		const std::pair<double, double>& lambdas = _lambdas[edges];

		int sumArcs = 0;
		for (const auto& edge : edges)
			sumArcs += mst[edge];

		double gradient1 = 2*_z[edges] - sumArcs;
		double gradient2 = sumArcs - _z[edges] - 1;

		if (gradient1 < 0 && lambdas.first < Configuration::LambdaEpsilon)
			gradient1 = 0;
		if (gradient2 < 0 && lambdas.second < Configuration::LambdaEpsilon)
			gradient2 = 0;

		feasible &= (gradient1 <= 0);
		feasible &= (gradient2 <= 0);

		// store the gradients in the same order we retrieved the lambdas
		*i = gradient1; i++;
		*i = gradient2; i++;

		LOG_ALL(meflog)
				<< "\t" << _graph << edges
				<< ": δλ¹ = " << gradient1
				<< ",\tδλ² = " << gradient2
				<< std::endl;
	}

	LOG_ALL(meflog) << std::endl;

	return feasible;
}

} // namespace detail
} // namespace host

#endif // HOST_INFERENCE_DETAIL_MULTI_FACTOR_TERM_IMPL_H__

