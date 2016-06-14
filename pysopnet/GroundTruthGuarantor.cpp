#include <pipeline/Value.h>
#include <pipeline/Process.h>
#include <blockwise/guarantors/GroundTruthGuarantor.h>
#include <blockwise/blocks/Cores.h>
#include <util/point.hpp>
#include "GroundTruthGuarantor.h"
#include "logging.h"

namespace python {

Locations
GroundTruthGuarantor::fill(
		const util::point<unsigned int, 3>& request,
		const GroundTruthGuarantorParameters& /*parameters*/,
		const ProjectConfiguration& configuration) {

	LOG_USER(pylog) << "[GroundTruthGuarantor] fill called for core at " << request << std::endl;

	boost::shared_ptr<SliceStore>   sliceStore    = createSliceStore(configuration, GroundTruth);
	boost::shared_ptr<SegmentStore> segmentStore  = createSegmentStore(configuration, GroundTruth);
	boost::shared_ptr<StackStore<LabelImage> > gtStackStore = createStackStore<LabelImage>(configuration, GroundTruth);

	// create the GroundTruthGuarantor process node
	::GroundTruthGuarantor groundTruthGuarantor(
			configuration,
			segmentStore,
			sliceStore,
			gtStackStore);

	// slice extraction parameters
	pipeline::Value<ComponentTreeExtractorParameters<LabelImage::value_type> > cteParameters;
	cteParameters->sameIntensityComponents = true;
	groundTruthGuarantor.setComponentTreeExtractorParameters(cteParameters);

	LOG_USER(pylog) << "[GroundTruthGuarantor] processing..." << std::endl;

	// find the core that corresponds to the request
	Core core(request.x(), request.y(), request.z());

	// let it do what it was build for
	Blocks missingBlocks = groundTruthGuarantor.guaranteeGroundTruth(core);

	LOG_USER(pylog) << "[GroundTruthGuarantor] collecting missing blocks" << std::endl;

	// collect missing block locations
	Locations missing;
	for (const Block& block : missingBlocks)
		missing.push_back(util::point<unsigned int, 3>(block.x(), block.y(), block.z()));

	return missing;
}

} // namespace python
