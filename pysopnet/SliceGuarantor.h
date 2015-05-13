#ifndef SOPNET_PYTHON_SLICE_GUARANTOR_H__
#define SOPNET_PYTHON_SLICE_GUARANTOR_H__

#include <blockwise/ProjectConfiguration.h>
#include <blockwise/BackendClient.h>
#include "SliceGuarantorParameters.h"

namespace python {

class SliceGuarantor : public BackendClient {

public:

	/**
	 * Request the extraction and storage of slices in a block.
	 *
	 * @param blockLocation
	 *             The location of the requested block.
	 *
	 * @param parameters
	 *             Slice extraction parameters.
	 *
	 * @param configuration
	 *             Project specific configuration.
	 */
	void fill(
			const util::point<unsigned int, 3>& blockLocation,
			const SliceGuarantorParameters& parameters,
			const ProjectConfiguration& configuration);
};

} // namespace python

#endif // SOPNET_PYTHON_SLICE_GUARANTOR_H__

