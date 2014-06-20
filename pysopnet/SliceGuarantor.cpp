#include <pipeline/Value.h>
#include <pipeline/Process.h>
#include <catmaid/SliceGuarantor.h>
#include <sopnet/block/Blocks.h>
#include "SliceGuarantor.h"
#include "logging.h"

namespace python {

struct MissingSliceData : virtual Exception {};

void
SliceGuarantor::fill(
		const point3<unsigned int>& request,
		const SliceGuarantorParameters& parameters,
		const ProjectConfiguration& configuration) {

	LOG_USER(pylog) << "[SliceGuarantor] fill called for block at " << request << std::endl;

	boost::shared_ptr<BlockManager> blockManager       = createBlockManager(configuration);
	boost::shared_ptr<StackStore>   membraneStackStore = createStackStore(configuration, Membrane);
	boost::shared_ptr<SliceStore>   sliceStore         = createSliceStore(configuration);

	// create a valid request block
	boost::shared_ptr<Block> requestBlock = blockManager->blockAtCoordinates(request);

	// wrap requested block into Blocks
	boost::shared_ptr<Blocks> blocks = boost::make_shared<Blocks>();
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

	LOG_DEBUG(pylog) << "[SliceGuarantor] asking for slices..." << std::endl;

	// let it do what it was build for
	pipeline::Value<Blocks> missing = sliceGuarantor->guaranteeSlices();

	LOG_DEBUG(pylog) << "[SliceGuarantor] " << missing->length() << " blocks missing" << std::endl;

	if (missing->length() > 0)
		UTIL_THROW_EXCEPTION(
				MissingSliceData,
				"not all images are available to extract slices in " << request);

	LOG_DEBUG(pylog) << "[SliceGuarantor] done" << std::endl;
}

} // namespace python
