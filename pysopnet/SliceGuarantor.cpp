#include <pipeline/Value.h>
#include <pipeline/Process.h>
#include <catmaid/SliceGuarantor.h>
#include <sopnet/block/Blocks.h>
#include "SliceGuarantor.h"
#include "logging.h"

namespace python {

void
SliceGuarantor::fill(
		const point3<unsigned int>& request,
		const SliceGuarantorParameters& parameters,
		const ProjectConfiguration& configuration) {

	LOG_USER(pylog) << "[SliceGuarantor] fill called for block at " << request << std::endl;

	pipeline::Value<CoreManager> blockManager       = createCoreManager(configuration);
	pipeline::Value<StackStore>   membraneStackStore = createStackStore(configuration);
	pipeline::Value<SliceStore>   sliceStore         = createSliceStore(configuration);

	// create a valid request block
	boost::shared_ptr<Block> requestBlock = blockManager->blockAtLocation(request);

	// wrap requested block into Blocks
	pipeline::Value<Blocks> blocks;
	blocks->add(requestBlock);

	// slice extraction parameters
	pipeline::Value<unsigned int> maxSliceSize(parameters.getMaxSliceSize());

	LOG_DEBUG(pylog) << "[SliceGuarantor] creating slice guarantor" << std::endl;

	// create the SliceGuarantor process node
	pipeline::Process< ::SliceGuarantor> sliceGuarantor;

	sliceGuarantor->setInput("blocks", blocks);
	sliceGuarantor->setInput("stack store", membraneStackStore);
	sliceGuarantor->setInput("slice store", sliceStore);
	sliceGuarantor->setInput("maximum area", maxSliceSize);

	// let it do what it was build for
	// NOTE: doesn't work, yet
	//sliceGuarantor->guaranteeSlices();

	LOG_DEBUG(pylog) << "[SliceGuarantor] done" << std::endl;
}

} // namespace python
