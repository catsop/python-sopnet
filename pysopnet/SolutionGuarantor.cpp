#include <pipeline/Value.h>
#include <pipeline/Process.h>
#include <blockwise/guarantors/SolutionGuarantor.h>
#include <blockwise/blocks/Cores.h>
#include <util/point.hpp>
#include "SolutionGuarantor.h"
#include "logging.h"

namespace python {

Locations
SolutionGuarantor::fill(
		const util::point<unsigned int, 3>& request,
		const SolutionGuarantorParameters& parameters,
		const ProjectConfiguration& configuration) {

	LOG_USER(pylog) << "[SolutionGuarantor] fill called for core at " << request << std::endl;

	//boost::shared_ptr<BlockManager> blockManager  = createBlockManager(configuration);
	boost::shared_ptr<SliceStore>   sliceStore    = createSliceStore(configuration);
	boost::shared_ptr<SegmentStore> segmentStore  = createSegmentStore(configuration);

	// create the SolutionGuarantor process node
	::SolutionGuarantor solutionGuarantor(
			configuration,
			segmentStore,
			sliceStore,
			parameters.getCorePadding(),
			parameters.forceExplanation(),
			parameters.readCosts(),
			parameters.storeCosts());

	LOG_USER(pylog) << "[SolutionGuarantor] processing..." << std::endl;

	// find the core that corresponds to the request
	Core core(request.x(), request.y(), request.z());

	// let it do what it was build for
	Blocks missingBlocks = solutionGuarantor.guaranteeSolution(core);

	LOG_USER(pylog) << "[SolutionGuarantor] collecting missing segment blocks" << std::endl;

	// collect missing block locations
	Locations missing;
	for (const Block& block : missingBlocks)
		missing.push_back(util::point<unsigned int, 3>(block.x(), block.y(), block.z()));

	return missing;
}

} // namespace python

