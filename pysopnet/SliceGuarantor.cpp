#include <pipeline/Value.h>
#include <pipeline/Process.h>
#include <catmaidsopnet/SliceGuarantor.h>
#include <catmaidsopnet/persistence/StackStore.h>
#include <catmaidsopnet/persistence/LocalStackStore.h>
#include <catmaidsopnet/persistence/SliceStore.h>
#include <catmaidsopnet/persistence/LocalSliceStore.h>
#include <sopnet/block/Blocks.h>
#include <sopnet/block/BlockManager.h>
#include <sopnet/block/LocalBlockManager.h>
#include "SliceGuarantor.h"
#include "logging.h"

namespace python {

void
SliceGuarantor::fill(
		const point3<unsigned int>& request,
		const SliceGuarantorParameters& parameters,
		const ProjectConfiguration& configuration) {

	LOG_USER(pylog) << "[SliceGuarantor] fill called for block at " << request << std::endl;

	pipeline::Value<BlockManager> blockManager       = createBlockManager(configuration);
	pipeline::Value<StackStore>   membraneStackStore = createStackStore(configuration);
	pipeline::Value<SliceStore>   sliceStore         = createSliceStore(configuration);

	// create a valid request block
	boost::shared_ptr<Block> requestBlock = blockManager->blockAtLocation(request);

	// wrap requested block into Blocks
	pipeline::Value<Blocks> blocks;
	blocks->add(requestBlock);

	// slice extraction parameters
	pipeline::Value<unsigned int> maxSliceSize(parameters.getMaxSliceSize());

	// create the SliceGuarantor process node
	pipeline::Process< ::SliceGuarantor> sliceGuarantor;

	sliceGuarantor->setInput("blocks", blocks);
	sliceGuarantor->setInput("membrane stack store", membraneStackStore);
	sliceGuarantor->setInput("slice store", sliceStore);
	sliceGuarantor->setInput("maximum area", maxSliceSize);

	// let it do what it was build for
	sliceGuarantor->guaranteeSlices();
}

pipeline::Value<BlockManager>
SliceGuarantor::createBlockManager(const ProjectConfiguration& configuration) {

	LOG_USER(pylog) << "[SliceGuarantor] create local block manager" << std::endl;

	// TODO: create one based on provided configuration
	pipeline::Value<LocalBlockManager> localBlockManager(
			LocalBlockManager(
					configuration.getVolumeSize(),
					configuration.getBlockSize()));

	return localBlockManager;
}

pipeline::Value<StackStore>
SliceGuarantor::createStackStore(const ProjectConfiguration& /*configuration*/) {

	LOG_USER(pylog) << "[SliceGuarantor] create local stack store for membranes" << std::endl;

	// TODO: create one based on provided configuration
	pipeline::Value<LocalStackStore> localStackStore(LocalStackStore("./membranes"));

	return localStackStore;
}

pipeline::Value<SliceStore>
SliceGuarantor::createSliceStore(const ProjectConfiguration& /*configuration*/) {

	LOG_USER(pylog) << "[SliceGuarantor] create local slice store" << std::endl;

	// TODO: create one based on provided configuration
	pipeline::Value<LocalSliceStore> localSliceStore;

	return localSliceStore;
}

} // namespace python
