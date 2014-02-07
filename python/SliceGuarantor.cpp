#include <pipeline/Value.h>
#include <pipeline/Process.h>
#include <catmaidsopnet/SliceGuarantor.h>
#include <catmaidsopnet/persistence/SliceStore.h>
#include <catmaidsopnet/persistence/LocalSliceStore.h>
#include <sopnet/block/BlockManager.h>
#include <sopnet/block/LocalBlockManager.h>
#include "SliceGuarantor.h"
#include "logging.h"

namespace python {

void
SliceGuarantor::fill(const Block& block, const SliceGuarantorParameters& parameters, const ProjectConfiguration& configuration) {

	LOG_USER(pylog) << "[SliceGuarantor] fill called for block " << block.location() << std::endl;

	// instantiate block manager
	// TODO: create one based on provided configuration
	pipeline::Value<LocalBlockManager> blockManager(
			LocalBlockManager(
					boost::make_shared<util::point3<unsigned int> >(configuration.getVolumeSize()),
					boost::make_shared<util::point3<unsigned int> >(configuration.getBlockSize())));

	// instantiate slice store
	// TODO: create one based on provided configuration
	pipeline::Value<LocalSliceStore> sliceStore;

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
}

} // namespace python
