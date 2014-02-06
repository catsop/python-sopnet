#ifndef SOPNET_PYTHON_SLICE_GUARANTOR_H__
#define SOPNET_PYTHON_SLICE_GUARANTOR_H__

#include <sopnet/block/Block.h>
#include "SliceGuarantorParameters.h"
#include "ProjectConfiguration.h"

namespace python {

class SliceGuarantor {

public:

	/**
	 * Request the extraction and storage of slices in a block.
	 */
	void fill(const Block& block, const SliceGuarantorParameters& paramters, const ProjectConfiguration& configuration);
};

} // namespace python

#endif // SOPNET_PYTHON_SLICE_GUARANTOR_H__

