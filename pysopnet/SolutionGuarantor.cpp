#include <pipeline/Value.h>
#include <pipeline/Process.h>
#include <catmaid/SolutionGuarantor.h>
#include <sopnet/block/Blocks.h>
#include "SolutionGuarantor.h"
#include "logging.h"

namespace python {

Locations
SolutionGuarantor::fill(
		const point3<unsigned int>& request,
		const SolutionGuarantorParameters& parameters,
		const ProjectConfiguration& configuration) {

	LOG_USER(pylog) << "[SolutionGuarantor] fill called for block at " << request << std::endl;

	pipeline::Value<CoreManager>  blockManager  = createCoreManager(configuration);
	pipeline::Value<SliceStore>    sliceStore    = createSliceStore(configuration);
	pipeline::Value<SegmentStore>  segmentStore  = createSegmentStore(configuration);
	pipeline::Value<SolutionStore> solutionStore = createSolutionStore(configuration);

	LOG_USER(pylog) << "[SolutionGuarantor] requesting block at " << request << std::endl;

	// create a valid request block
	boost::shared_ptr<Block> requestBlock = blockManager->blockAtLocation(request);

	// wrap requested block into Blocks
	pipeline::Value<Blocks> blocks;
	blocks->add(requestBlock);

	// create the SolutionGuarantor process node
	pipeline::Process< ::SolutionGuarantor> solutionGuarantor;

	LOG_USER(pylog) << "[SolutionGuarantor] setting inputs" << std::endl;

	// TODO:
	// there might be more inputs that need to be set
	//solutionGuarantor->setInput("blocks", blocks);
	//solutionGuarantor->setInput("slice store", sliceStore);
	//solutionGuarantor->setInput("segment store", segmentStore);
	//solutionGuarantor->setInput("solution store", solutionStore);

	LOG_USER(pylog) << "[SolutionGuarantor] processing..." << std::endl;

	// let it do what it was build for
	//solutionGuarantor->guaranteeSolutions();

	LOG_USER(pylog) << "[SolutionGuarantor] collecting missing segment blocks" << std::endl;

	// TODO:
	// read output of missing slice blocks
	Locations missing;
	missing.push_back(util::point3<unsigned int>(1,2,3));
	missing.push_back(util::point3<unsigned int>(3,2,1));

	return missing;
}

} // namespace python

