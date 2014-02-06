#ifndef SOPNET_PYTHON_SLICE_GUARANTOR_PARAMETERS_H__
#define SOPNET_PYTHON_SLICE_GUARANTOR_PARAMETERS_H__

namespace python {

class SliceGuarantorParameters {

public:

	SliceGuarantorParameters() :
		_forceExplanation(false) {}

	/**
	 * Create constraints in the slices, such that on each path in the component 
	 * tree exactly one slice has to be taken. This forces every compenent tree 
	 * to be involved in a neuron.
	 */
	void setForceExplanation(bool force) {

		_forceExplanation = force;
	}

	bool getForceExplanation() const {

		return _forceExplanation;
	}

	/**
	 * Set the maximal size of slices that will be extracted.
	 */
	void setMaxSliceSize(unsigned int max) {

		_maxSliceSize = max;
	}

	unsigned int getMaxSliceSize() const {

		return _maxSliceSize;
	}

private:

	bool _forceExplanation;

	unsigned int _maxSliceSize;
};

} // namespace python

#endif // SOPNET_PYTHON_SLICE_GUARANTOR_PARAMETERS_H__

