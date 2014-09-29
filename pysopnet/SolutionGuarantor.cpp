#include <pipeline/Value.h>
#include <pipeline/Process.h>
#include <catmaid/guarantors/SolutionGuarantor.h>
#include <catmaid/blocks/Cores.h>
#include "SolutionGuarantor.h"
#include "logging.h"

namespace python {

Locations
SolutionGuarantor::fill(
		const point3<unsigned int>& request,
		const SolutionGuarantorParameters& parameters,
		const ProjectConfiguration& configuration) {

	LOG_USER(pylog) << "[SolutionGuarantor] fill called for core at " << request << std::endl;

	boost::shared_ptr<BlockManager> blockManager  = createBlockManager(configuration);
	boost::shared_ptr<SliceStore>   sliceStore    = createSliceStore(configuration);
	boost::shared_ptr<SegmentStore> segmentStore  = createSegmentStore(configuration);

	// create the SolutionGuarantor process node
	::SolutionGuarantor solutionGuarantor(
			segmentStore,
			sliceStore,
			parameters.getCorePadding(),
			std::vector<double>() /* TODO: read from configuration store */);

	LOG_USER(pylog) << "[SolutionGuarantor] processing..." << std::endl;

	// find the core that corresponds to the request
	boost::shared_ptr<Core> core = blockManager->coreAtCoordinates(request);

	// let it do what it was build for
	Blocks missingBlocks = solutionGuarantor.guaranteeSolution(*core);

	LOG_USER(pylog) << "[SolutionGuarantor] collecting missing segment blocks" << std::endl;

	// collect missing block locations
	Locations missing;
	foreach (boost::shared_ptr<Block> block, missingBlocks)
		missing.push_back(block->getCoordinates());

	return missing;
}

} // namespace python

