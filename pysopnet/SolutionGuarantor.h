#ifndef SOPNET_PYTHON_SOLUTION_GUARANTOR_H__
#define SOPNET_PYTHON_SOLUTION_GUARANTOR_H__

#include "SolutionGuarantorParameters.h"
#include "ProjectConfiguration.h"
#include "BackendClient.h"
#include "Locations.h"

namespace python {

class SolutionGuarantor : public BackendClient {

public:

	/**
	 * Request the extraction and storage of solutions in a block.
	 *
	 * @param blockLocation
	 *             The location of the requested block.
	 *
	 * @param parameters
	 *             Solution extraction parameters.
	 *
	 * @param configuration
	 *             Project specific configuration.
	 *
	 * @return
	 *             A list of block locations, for which segments are needed to 
	 *             process the request. Empty on success.
	 */
	Locations fill(
			const util::point3<unsigned int>& blockLocation,
			const SolutionGuarantorParameters& parameters,
			const ProjectConfiguration& configuration);
};

} // namespace python

#endif // SOPNET_PYTHON_SOLUTION_GUARANTOR_H__

