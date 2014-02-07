#include <pipeline/Value.h>
#include <pipeline/Process.h>
#include <catmaidsopnet/SliceGuarantor.h>
#include <catmaidsopnet/persistence/StackStore.h>
#include <catmaidsopnet/persistence/LocalStackStore.h>
#include <catmaidsopnet/persistence/SliceStore.h>
#include <catmaidsopnet/persistence/LocalSliceStore.h>
#include <sopnet/block/BlockManager.h>
#include <sopnet/block/LocalBlockManager.h>
#include "SliceGuarantor.h"
#include "logging.h"

namespace python {

void
SliceGuarantor::fill(
		const Block& block,
		const SliceGuarantorParameters& parameters,
		const ProjectConfiguration& configuration) {

	LOG_USER(pylog) << "[SliceGuarantor] fill called for block " << block.location() << std::endl;

	pipeline::Value<BlockManager> blockManager = createBlockManager(configuration);
	pipeline::Value<StackStore>   stackStore   = createStackStore(configuration);
	pipeline::Value<SliceStore>   sliceStore   = createSliceStore(configuration);

	// instantiate image block factory
	pipeline::Value<ImageBlockFactory> membraneBlockFactory;

	// slice extraction parameters
	pipeline::Value<bool> forceExplanation(parameters.getForceExplanation());
	pipeline::Value<unsigned int> maxSliceSize(parameters.getMaxSliceSize());

	// create the SliceGuarantor process node
	pipeline::Process< ::SliceGuarantor> sliceGuarantor;

	sliceGuarantor->setInput("block manager", blockManager);
	sliceGuarantor->setInput("store", sliceStore);
	sliceGuarantor->setInput("block factory", membraneBlockFactory);
	sliceGuarantor->setInput("force explanation", forceExplanation);
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
					boost::make_shared<util::point3<unsigned int> >(configuration.getVolumeSize()),
					boost::make_shared<util::point3<unsigned int> >(configuration.getBlockSize())));

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
