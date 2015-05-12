#ifndef HOST_INFERENCE_DETAIL_EXCLUSIVE_TERM_IMPL_H__
#define HOST_INFERENCE_DETAIL_EXCLUSIVE_TERM_IMPL_H__

#include <ostream>
#include <util/helpers.hpp>
#include <graph/Logging.h>
#include <inference/HigherOrderArcTerm.h>
#include <inference/Configuration.h>

namespace host {
namespace detail {

template <typename EdgeType>
class ExclusiveTermImpl : public HigherOrderArcTerm {

public:

	ExclusiveTermImpl(
			EdgeType edge1,
			EdgeType edge2) :
		_edge1(edge1),
		_edge2(edge2),
		_lambdas(4, 0) {}

	/**
	 * Get the number of lambda parameters of this higher order term.
	 */
	size_t numLambdas() const { return 4; }

	/**
	 * Change the upper and lower bounds for each lambda in the the given 
	 * ranges. The defaults are -inf for the lower bounds and +inf for the upper 
	 * bounds.
	 */
	void lambdaBounds(
			Lambdas::iterator,
			Lambdas::iterator,
			Lambdas::iterator,
			Lambdas::iterator) { /* no bounds */ }

	/**
	 * Set the lambda parameters.
	 */
	void setLambdas(
			Lambdas::const_iterator begin,
			Lambdas::const_iterator end) {

		std::copy(begin, end, _lambdas.begin());
		optimize();
	}

	/**
	 * Add the lambda contributions of this higher order term to the given arc 
	 * weights.
	 */
	void addArcWeights(host::ArcWeights& weights) {

		// Consider the unary costs
		//
		//  t(x=0) and t(x=1), where we have t(x=0)=0
		//
		// Our lambdas contribute the them in the following way:
		//
		//  t'(x=0) = t(x=0) - l(x=0)
		//  t'(x=1) = t(x=1) - l(x=1)
		//
		// We want to adjust our contributions, such that t'(x=0)=0. For that, 
		// we define
		//
		//  l'(x=0) = 0
		//  l'(x=1) = l(x=1) - l(x=0),
		//
		// that is, we subtracted l(x=0) from our contribution. Since our 
		// contribution is negated, that means we added a constant of l(x=0) to 
		// the unaries:
		//
		//  t''(x=0) = t(x=0) - l'(x=0) = t(x=0) - l(x=0) + l(x=0)
		//                              = t(x=0)
		//                              nothing to do
		//
		//  t''(x=1) = t(x=1) - l'(x=1) = t(x=1) - l(x=1) + l(x=0)
		//                              = t(x=1) - [l(x=1) - l(x=0)]
		//                              = t(x=1) + [l(x=0) - l(x=1)]
		//                              that's what we do here:

		weights[_edge1] += Edge1Value0() - Edge1Value1();
		weights[_edge2] += Edge2Value0() - Edge2Value1();

		// In constant() of this term, we have to compensate for the introduced 
		// constant l(x=0)
	}

	/**
	 * Get the constant contribution of this higher order term to the objective.
	 */
	double constant() {

		double constant = 0;

		// The constant contribution is the minimal value obtained by optimize()
		//
		//   min v(e,f) = min lambda1(e) + lambda2(f)
		//
		// minus the constant that we introduced by setting the unaries above.

		constant += (_edge1Selected ? Edge1Value1() : Edge1Value0());
		constant += (_edge2Selected ? Edge2Value1() : Edge2Value0());

		constant -= Edge1Value0();
		constant -= Edge2Value0();

		return constant;
	}

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
			Lambdas::iterator         end) {

		bool feasible = (mst[_edge1] + mst[_edge2] <= 1);

		// the value of this term under the current mst
		double mstValue =
				(mst[_edge1] ? Edge1Value1() : Edge1Value0() ) +
				(mst[_edge1] ? Edge2Value1() : Edge2Value0() );

		// if the mst is feasible (according to this term) and the solution is 
		// very close to our min, there is no need to continue searching
		if (feasible && std::abs(mstValue - _value) < Configuration::TermEps) {

			for (Lambdas::iterator i = begin; i != end; i++)
				*i = 0;

			return true;
		}

		// the gradient is the disagreement between our term and the current mst
		double g10 = (_edge1Selected == false) - (mst[_edge1] == false);
		double g11 = (_edge1Selected == true ) - (mst[_edge1] == true );
		double g20 = (_edge2Selected == false) - (mst[_edge2] == false);
		double g21 = (_edge2Selected == true ) - (mst[_edge2] == true );

		Lambdas::iterator i = begin;
		(*i) = g10; i++;
		(*i) = g11; i++;
		(*i) = g20; i++;
		(*i) = g21;

		return feasible;
	}

	EdgeType edge1() const { return _edge1; }
	EdgeType edge2() const { return _edge2; }

	const Lambdas& lambdas() const { return _lambdas; }

	bool operator==(const ExclusiveTermImpl<EdgeType>& other) const {

		return ((_edge1 == other._edge1) && (_edge2 == other._edge2)) ||
		       ((_edge2 == other._edge1) && (_edge1 == other._edge2));
	}

private:

	void optimize() {

		// costs for selecting edges 1 and 2
		double w1 = Edge1Value1() - Edge1Value0();
		double w2 = Edge2Value1() - Edge2Value0();

		_edge1Selected = false;
		_edge2Selected = false;

		// four cases:
		//   both are positive -> select none
		//   one is positive   -> select negative = select minimum
		//   none is positive  -> select minimum
		if (w1 < 0 || w2 < 0) {

			if (w1 < w2) {

				_edge1Selected = true;
				_value = Edge1Value1() + Edge2Value0();

			} else {

				_edge2Selected = true;
				_value = Edge1Value0() + Edge2Value1();
			}
		} else {

			_value = Edge1Value0() + Edge2Value0();
		}
	}

	inline double& Edge1Value0() { return _lambdas[0]; }
	inline double& Edge1Value1() { return _lambdas[1]; }
	inline double& Edge2Value0() { return _lambdas[2]; }
	inline double& Edge2Value1() { return _lambdas[3]; }

	// the mutually exclusive edges
	EdgeType _edge1, _edge2;

	// the minimal configuration of this term
	bool _edge1Selected, _edge2Selected;

	// the minimal value of this term
	double _value;

	// lambdas assicoated to this term
	Lambdas _lambdas;
};

template <typename EdgeType>
std::ostream& operator<<(std::ostream& out, const ExclusiveTermImpl<EdgeType>& term) {

	out << term.edge1() << " -- " << term.edge2() << "\t" << term.lambdas();
	return out;
}

} // namespace detail
} // namespace host

#endif // HOST_INFERENCE_DETAIL_EXCLUSIVE_TERM_IMPL_H__


