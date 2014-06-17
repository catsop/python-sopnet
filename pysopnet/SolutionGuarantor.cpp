#include <pipeline/Value.h>
#include <pipeline/Process.h>
#include <catmaid/SolutionGuarantor.h>
#include <sopnet/block/Cores.h>
#include <sopnet/block/CoreManager.h>
#include "SolutionGuarantor.h"
#include "logging.h"

namespace python {

Locations
SolutionGuarantor::fill(
		const point3<unsigned int>& request,
		const SolutionGuarantorParameters& parameters,
		const ProjectConfiguration& configuration) {

	LOG_USER(pylog) << "[SolutionGuarantor] fill called for block at " << request << std::endl;

	pipeline::Value<BlockManager>  blockManager  = createBlockManager(configuration);
	pipeline::Value<StackStore>    rawStackStore = createStackStore(configuration, Raw);
	pipeline::Value<SliceStore>    sliceStore    = createSliceStore(configuration);
	pipeline::Value<SegmentStore>  segmentStore  = createSegmentStore(configuration);
	pipeline::Value<SolutionStore> solutionStore = createSolutionStore(configuration);

	LOG_USER(pylog) << "[SolutionGuarantor] requesting block at " << request << std::endl;

	// find the cores that correspond to request
	CoreManager coreManager(blockManager, configuration.getCoreSize());
	boost::shared_ptr<Core> core = coreManager.coreAtLocation(request);
	pipeline::Value<Cores> cores;
	cores->add(core);

	// create the SolutionGuarantor process node
	pipeline::Process< ::SolutionGuarantor> solutionGuarantor;

	LOG_USER(pylog) << "[SolutionGuarantor] setting inputs" << std::endl;

	pipeline::Value<bool>         forceExplanation(parameters.forceExplanation());
	pipeline::Value<unsigned int> corePadding(parameters.getCorePadding());

	solutionGuarantor->setInput("cores", cores);
	solutionGuarantor->setInput("segment store", segmentStore);
	solutionGuarantor->setInput("slice store", sliceStore);
	solutionGuarantor->setInput("raw image store", rawStackStore);
	solutionGuarantor->setInput("force explanation", forceExplanation);
	solutionGuarantor->setInput("buffer", corePadding);

	LOG_USER(pylog) << "[SolutionGuarantor] processing..." << std::endl;

	// let it do what it was build for
	pipeline::Value<Blocks> missingBlocks = solutionGuarantor->guaranteeSolution();

	LOG_USER(pylog) << "[SolutionGuarantor] collecting missing segment blocks" << std::endl;

	// collect missing block locations
	Locations missing;
	foreach (boost::shared_ptr<Block> block, *missingBlocks)
		missing.push_back(block->getCoordinates());

	return missing;
}

} // namespace python

