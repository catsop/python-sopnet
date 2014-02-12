#ifndef SOPNET_PYTHON_SLICE_GUARANTOR_H__
#define SOPNET_PYTHON_SLICE_GUARANTOR_H__

#include "SliceGuarantorParameters.h"
#include "ProjectConfiguration.h"
#include "BackendClient.h"

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
			const util::point3<unsigned int>& blockLocation,
			const SliceGuarantorParameters& parameters,
			const ProjectConfiguration& configuration);
};

} // namespace python

#endif // SOPNET_PYTHON_SLICE_GUARANTOR_H__

