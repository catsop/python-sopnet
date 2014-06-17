#include <pipeline/Value.h>
#include <pipeline/Process.h>
#include <catmaid/SegmentGuarantor.h>
#include <sopnet/block/Blocks.h>
#include "SegmentGuarantor.h"
#include "logging.h"

namespace python {

Locations
SegmentGuarantor::fill(
		const point3<unsigned int>& request,
		const SegmentGuarantorParameters& /*parameters*/,
		const ProjectConfiguration& configuration) {

	LOG_USER(pylog) << "[SegmentGuarantor] fill called for block at " << request << std::endl;

	boost::shared_ptr<BlockManager> blockManager = createBlockManager(configuration);
	boost::shared_ptr<SliceStore>   sliceStore   = createSliceStore(configuration);
	boost::shared_ptr<SegmentStore> segmentStore = createSegmentStore(configuration);

	// create a valid request block
	boost::shared_ptr<Block> requestBlock = blockManager->blockAtLocation(request);

	// wrap requested block into Blocks
	boost::shared_ptr<Blocks> blocks = boost::make_shared<Blocks>();
	blocks->add(requestBlock);

	// create the SegmentGuarantor process node
	pipeline::Process< ::SegmentGuarantor> segmentGuarantor;

	segmentGuarantor->setInput("blocks", blocks);
	segmentGuarantor->setInput("slice store", sliceStore);
	segmentGuarantor->setInput("segment store", segmentStore);

	// let it do what it was build for
	pipeline::Value<Blocks> missingBlocks = segmentGuarantor->guaranteeSegments();

	// collect missing block locations
	Locations missing;
	foreach (boost::shared_ptr<Block> block, *missingBlocks)
		missing.push_back(block->getCoordinates());

	return missing;
}

} // namespace python
