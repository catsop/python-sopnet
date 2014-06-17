#ifndef SOPNET_PYTHON_SOLUTION_GUARANTOR_PARAMETERS_H__
#define SOPNET_PYTHON_SOLUTION_GUARANTOR_PARAMETERS_H__

namespace python {

class SolutionGuarantorParameters {

public:

	SolutionGuarantorParameters() :
		_forceExplanation(false),
		_corePadding(2) {}

	/**
	 * Should every clique in the slice conflict graph provide exactly one slice 
	 * to the reconstruction? If false, slice candidates can be ignored.
	 */
	bool forceExplanation() const { return _forceExplanation; }

	/**
	 * Should every clique in the slice conflict graph provide exactly one slice 
	 * to the reconstruction? If false, slice candidates can be ignored.
	 */
	void setForceExplanation(bool forceExplanation) { _forceExplanation = forceExplanation; }

	/**
	 * Get the number of blocks to pad around the core to get near-optimal 
	 * solutions.
	 */
	unsigned int getCorePadding() const { return _corePadding; }

	/**
	 * Set the number of blocks to pad around the core to get near-optimal 
	 * solutions.
	 */
	void setCorePadding(unsigned int corePadding) { _corePadding = corePadding; }

private:

	bool _forceExplanation;

	unsigned int _corePadding;
};

} // namespace python

#endif // SOPNET_PYTHON_SOLUTION_GUARANTOR_PARAMETERS_H__

