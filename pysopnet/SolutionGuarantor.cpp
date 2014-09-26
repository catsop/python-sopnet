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

	LOG_USER(pylog) << "[SolutionGuarantor] fill called for block at " << request << std::endl;

	boost::shared_ptr<BlockManager>  blockManager  = createBlockManager(configuration);
	boost::shared_ptr<StackStore>    rawStackStore = createStackStore(configuration, Raw);
	boost::shared_ptr<SliceStore>    sliceStore    = createSliceStore(configuration);
	boost::shared_ptr<SegmentStore>  segmentStore  = createSegmentStore(configuration);
	//boost::shared_ptr<SolutionStore> solutionStore = createSolutionStore(configuration);

	LOG_USER(pylog) << "[SolutionGuarantor] requesting block at " << request << std::endl;

	// find the cores that correspond to request
	boost::shared_ptr<Core> core = blockManager->coreAtCoordinates(request);
	boost::shared_ptr<Cores> cores = boost::make_shared<Cores>();
	cores->add(core);

	// create the SolutionGuarantor process node
	pipeline::Process< ::SolutionGuarantor> solutionGuarantor;

	LOG_USER(pylog) << "[SolutionGuarantor] setting inputs" << std::endl;

	pipeline::Value<bool>         forceExplanation(parameters.forceExplanation());
	pipeline::Value<unsigned int> corePadding(parameters.getCorePadding());

	solutionGuarantor->setInput("cores", cores);
	// slice store is not a pipeline::Data any more, solutionGuarantor will not 
	// be a process node soon
	//solutionGuarantor->setInput("segment store", segmentStore);
	// slice store is not a pipeline::Data any more, solutionGuarantor will not 
	// be a process node soon
	//solutionGuarantor->setInput("slice store", sliceStore);
	solutionGuarantor->setInput("raw stack store", rawStackStore);
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

