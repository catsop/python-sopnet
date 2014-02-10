#ifndef SOPNET_PYTHON_SLICE_GUARANTOR_PARAMETERS_H__
#define SOPNET_PYTHON_SLICE_GUARANTOR_PARAMETERS_H__

#include <limits>

namespace python {

class SliceGuarantorParameters {

public:

	SliceGuarantorParameters() :
		_maxSliceSize(std::numeric_limits<unsigned int>::max()) {}

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

	unsigned int _maxSliceSize;
};

} // namespace python

#endif // SOPNET_PYTHON_SLICE_GUARANTOR_PARAMETERS_H__

