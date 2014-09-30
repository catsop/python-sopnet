#ifndef SOPNET_PYTHON_SEGMENT_GUARANTOR_H__
#define SOPNET_PYTHON_SEGMENT_GUARANTOR_H__

#include <catmaid/ProjectConfiguration.h>
#include "SegmentGuarantorParameters.h"
#include "BackendClient.h"
#include "Locations.h"

namespace python {

class SegmentGuarantor : public BackendClient {

public:

	/**
	 * Request the extraction and storage of segments in a block.
	 *
	 * @param blockLocation
	 *             The location of the requested block.
	 *
	 * @param parameters
	 *             Segment extraction parameters.
	 *
	 * @param configuration
	 *             Project specific configuration.
	 *
	 * @return
	 *             A list of block locations, for which slices are needed to 
	 *             process the request. Empty on success.
	 */
	Locations fill(
			const util::point3<unsigned int>& blockLocation,
			const SegmentGuarantorParameters& parameters,
			const ProjectConfiguration& configuration);
};

} // namespace python

#endif // SOPNET_PYTHON_SEGMENT_GUARANTOR_H__

