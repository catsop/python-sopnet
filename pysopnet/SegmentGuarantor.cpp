#include <catmaid/guarantors/SegmentGuarantor.h>
#include <catmaid/blocks/Blocks.h>
#include "SegmentGuarantor.h"
#include "logging.h"

namespace python {

Locations
SegmentGuarantor::fill(
		const point3<unsigned int>& request,
		const SegmentGuarantorParameters& /*parameters*/,
		const ProjectConfiguration& configuration) {

	LOG_USER(pylog) << "[SegmentGuarantor] fill called for block at " << request << std::endl;

	boost::shared_ptr<StackStore>   rawStackStore = createStackStore(configuration, Raw);
	boost::shared_ptr<BlockManager> blockManager = createBlockManager(configuration);
	boost::shared_ptr<SliceStore>   sliceStore   = createSliceStore(configuration);
	boost::shared_ptr<SegmentStore> segmentStore = createSegmentStore(configuration);

	// create a valid request block
	boost::shared_ptr<Block> requestBlock = blockManager->blockAtCoordinates(request);

	// wrap requested block into Blocks
	boost::shared_ptr<Blocks> blocks = boost::make_shared<Blocks>();
	blocks->add(requestBlock);

	// create a SegmentGuarantor
	::SegmentGuarantor segmentGuarantor;

	segmentGuarantor.setSegmentStore(segmentStore);
	segmentGuarantor.setSliceStore(sliceStore);
	segmentGuarantor.setRawStackStore(rawStackStore);

	// let it do what it was build for
	Blocks missingBlocks = segmentGuarantor.guaranteeSegments(*blocks);

	// collect missing block locations
	Locations missing;
	foreach (boost::shared_ptr<Block> block, missingBlocks)
		missing.push_back(block->getCoordinates());

	return missing;
}

} // namespace python
