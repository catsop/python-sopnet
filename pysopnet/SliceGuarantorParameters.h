#ifndef SOPNET_PYTHON_SLICE_GUARANTOR_PARAMETERS_H__
#define SOPNET_PYTHON_SLICE_GUARANTOR_PARAMETERS_H__

#include <limits>

namespace python {

class SliceGuarantorParameters {

public:

	SliceGuarantorParameters() :
		_minSliceSize(100),
		_maxSliceSize(100000),
		_membraneIsBright(true)
		{}

	/**
	 * Set the maximal size of slices that will be extracted.
	 */
	void setMaxSliceSize(unsigned int max) {

		_maxSliceSize = max;
	}

	/**
	 * Get the maximal size of slices that will be extracted.
	 */
	unsigned int getMaxSliceSize() const {

		return _maxSliceSize;
	}

	/**
	 * Set the minimal size of slices that will be extracted.
	 */
	void setMinSliceSize(unsigned int min) {

		_minSliceSize = min;
	}

	/**
	 * Get the minimal size of slices that will be extracted.
	 */
	unsigned int getMinSliceSize() const {

		return _minSliceSize;
	}

	/**
	 * Indicates that the membrane is bright in the membrane images, i.e., 
	 * slices are extracted from dark to bright.
	 */
	bool membraneIsBright() const {

		return _membraneIsBright;
	}

	/**
	 * Indicate that the membrane is bright in the membrane images, i.e., slices 
	 * are extracted from dark to bright.
	 */
	void setMembraneIsBright(bool isBright = true) {

		_membraneIsBright = isBright;
	}

private:

	unsigned int _minSliceSize;
	unsigned int _maxSliceSize;

	bool _membraneIsBright;
};

} // namespace python

#endif // SOPNET_PYTHON_SLICE_GUARANTOR_PARAMETERS_H__

