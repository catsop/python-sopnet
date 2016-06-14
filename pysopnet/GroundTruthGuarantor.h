#ifndef SOPNET_PYTHON_GROUND_TRUTH_GUARANTOR_H__
#define SOPNET_PYTHON_GROUND_TRUTH_GUARANTOR_H__

#include <blockwise/ProjectConfiguration.h>
#include <blockwise/persistence/BackendClient.h>
#include "GroundTruthGuarantorParameters.h"
#include "Locations.h"

namespace python {

class GroundTruthGuarantor : public BackendClient {

public:

	/**
	 * Request the extraction and storage of slices, segments, and a
	 * solution reflected a ground truth labeling.
	 *
	 * @param coreLocation
	 *             The location of the requested core.
	 *
	 * @param parameters
	 *             Solution extraction parameters.
	 *
	 * @param configuration
	 *             Project specific configuration.
	 *
	 * @return
	 *             A list of block locations, for which additional data is
	 *             needed to process the request. Empty on success.
	 */
	Locations fill(
			const util::point<unsigned int, 3>& blockLocation,
			const GroundTruthGuarantorParameters& parameters,
			const ProjectConfiguration& configuration);
};

} // namespace python

#endif // SOPNET_PYTHON_GROUND_TRUTH_GUARANTOR_H__
