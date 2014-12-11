#ifndef SOPNET_PYTHON_SOLUTION_GUARANTOR_PARAMETERS_H__
#define SOPNET_PYTHON_SOLUTION_GUARANTOR_PARAMETERS_H__

namespace python {

class SolutionGuarantorParameters {

public:

	SolutionGuarantorParameters() :
		_forceExplanation(false),
		_readCosts(false),
		_storeCosts(true),
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
	 * If true, cached costs available from the SegmentStore are used instead of
	 * features and weights where available.
	 */
	bool readCosts() const { return _readCosts; }

	/**
	 * If true, cached costs available from the SegmentStore are used instead of
	 * features and weights where available.
	 */
	void setReadCosts(bool readCosts) { _readCosts = readCosts; }

	/**
	 * If true, computed segment costs are saved to SegmentStore.
	 */
	bool storeCosts() const { return _storeCosts; }

	/**
	 * If true, computed segment costs are saved to SegmentStore.
	 */
	void setStoreCosts(bool storeCosts) { _storeCosts = storeCosts; }

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
	bool _readCosts;
	bool _storeCosts;

	unsigned int _corePadding;
};

} // namespace python

#endif // SOPNET_PYTHON_SOLUTION_GUARANTOR_PARAMETERS_H__

