#include <pipeline/Value.h>
#include <pipeline/Process.h>
#include <catmaid/guarantors/SliceGuarantor.h>
#include <catmaid/blocks/Blocks.h>
#include <util/point3.hpp>
#include "SliceGuarantor.h"
#include "logging.h"

namespace python {

struct MissingSliceData : virtual Exception {};

void
SliceGuarantor::fill(
		const util::point3<unsigned int>& request,
		const SliceGuarantorParameters& parameters,
		const ProjectConfiguration& configuration) {

	LOG_USER(pylog) << "[SliceGuarantor] fill called for block at " << request << std::endl;

	//boost::shared_ptr<BlockManager> blockManager       = createBlockManager(configuration);
	boost::shared_ptr<StackStore>   membraneStackStore = createStackStore(configuration, Membrane);
	boost::shared_ptr<SliceStore>   sliceStore         = createSliceStore(configuration);

	// create a valid request block
	Block requestBlock(request.x, request.y, request.z);

	// wrap requested block into Blocks
	Blocks blocks;
	blocks.add(requestBlock);

	LOG_DEBUG(pylog) << "[SliceGuarantor] creating slice guarantor" << std::endl;

	// create the SliceGuarantor
	::SliceGuarantor sliceGuarantor(sliceStore, membraneStackStore);

	// slice extraction parameters
	pipeline::Value<MserParameters> mserParameters;
	mserParameters->darkToBright =  parameters.membraneIsBright();
	mserParameters->brightToDark = !parameters.membraneIsBright();
	mserParameters->minArea      =  parameters.getMinSliceSize();
	mserParameters->maxArea      =  parameters.getMaxSliceSize();
	mserParameters->fullComponentTree = true;
	sliceGuarantor.setMserParameters(mserParameters);

	LOG_DEBUG(pylog) << "[SliceGuarantor] asking for slices..." << std::endl;

	// let it do what it was build for
	Blocks missing = sliceGuarantor.guaranteeSlices(blocks);

	LOG_DEBUG(pylog) << "[SliceGuarantor] " << missing.size() << " blocks missing" << std::endl;

	if (missing.size() > 0)
		UTIL_THROW_EXCEPTION(
				MissingSliceData,
				"not all images are available to extract slices in " << request);

	LOG_DEBUG(pylog) << "[SliceGuarantor] done" << std::endl;
}

} // namespace python
