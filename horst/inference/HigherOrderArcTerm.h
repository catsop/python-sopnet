#ifndef HOST_INFERENCE_HIGHER_ORDER_TERM_H__
#define HOST_INFERENCE_HIGHER_ORDER_TERM_H__

#include "ArcTerm.h"
#include "Lambdas.h"

/**
 * A term on multiple arcs. Also known as factor or k-ary interaction.
 *
 * Each term realizes a function on the variables it spans. It interacts with 
 * the DMST problem via lambdas that contribute to the arc weights, thus it is 
 * an ArcTerm.
 */
class HigherOrderArcTerm : public ArcTerm {

public:

	/**
	 * Get the number of lambda parameters of this higher order term.
	 */
	virtual size_t numLambdas() const = 0;

	/**
	 * Change the upper and lower bounds for each lambda in the the given 
	 * ranges. The defaults are -inf for the lower bounds and +inf for the upper 
	 * bounds.
	 */
	virtual void lambdaBounds(
			Lambdas::iterator beginLower,
			Lambdas::iterator endLower,
			Lambdas::iterator beginUpper,
			Lambdas::iterator endUpper) = 0;

	/**
	 * Set the lambda parameters.
	 */
	virtual void setLambdas(Lambdas::const_iterator begin, Lambdas::const_iterator end) = 0;

	/**
	 * Add the lambda contributions of this higher order term to the given arc 
	 * weights.
	 */
	virtual void addArcWeights(host::ArcWeights& weights) = 0;

	/**
	 * Get the constant contribution of this higher order term to the objective.
	 */
	virtual double constant() = 0;

	/**
	 * For the given MST (represented as boolean flags on arcs), compute the 
	 * gradient for each lambda and store it in the range pointed to with the 
	 * given iterator.
	 *
	 * @return true, if the current mst is feasible
	 */
	virtual bool gradient(
			const host::ArcSelection& mst,
			Lambdas::iterator         begin,
			Lambdas::iterator         end) = 0;
};

#endif // HOST_INFERENCE_HIGHER_ORDER_TERM_H__

