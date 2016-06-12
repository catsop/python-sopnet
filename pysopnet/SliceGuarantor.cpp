#include <pipeline/Value.h>
#include <pipeline/Process.h>
#include <blockwise/guarantors/SliceGuarantor.h>
#include <blockwise/blocks/Blocks.h>
#include <util/point.hpp>
#include "SliceGuarantor.h"
#include "logging.h"

namespace python {

struct MissingSliceData : virtual Exception {};

void
SliceGuarantor::fill(
		const util::point<unsigned int, 3>& request,
		const SliceGuarantorParameters& parameters,
		const ProjectConfiguration& configuration) {

	LOG_USER(pylog) << "[SliceGuarantor] fill called for block at " << request << std::endl;

	//boost::shared_ptr<BlockManager> blockManager       = createBlockManager(configuration);
	boost::shared_ptr<StackStore<IntensityImage> >   membraneStackStore = createStackStore<IntensityImage>(configuration, Membrane);
	boost::shared_ptr<SliceStore>   sliceStore         = createSliceStore(configuration);

	// create a valid request block
	Block requestBlock(request.x(), request.y(), request.z());

	// wrap requested block into Blocks
	Blocks blocks;
	blocks.add(requestBlock);

	LOG_DEBUG(pylog) << "[SliceGuarantor] creating slice guarantor" << std::endl;

	// create the SliceGuarantor
	::SliceGuarantor sliceGuarantor(configuration, sliceStore, membraneStackStore);

	// slice extraction parameters
	pipeline::Value<ComponentTreeExtractorParameters<IntensityImage::value_type> > cteParameters;
	cteParameters->darkToBright =  parameters.membraneIsBright();
	cteParameters->minSize      =  parameters.getMinSliceSize();
	cteParameters->maxSize      =  parameters.getMaxSliceSize();
	sliceGuarantor.setComponentTreeExtractorParameters(cteParameters);

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
