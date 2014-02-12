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
		const SegmentGuarantorParameters& parameters,
		const ProjectConfiguration& configuration) {

	LOG_USER(pylog) << "[SegmentGuarantor] fill called for block at " << request << std::endl;

	pipeline::Value<BlockManager> blockManager = createBlockManager(configuration);
	pipeline::Value<SliceStore>   sliceStore   = createSliceStore(configuration);
	pipeline::Value<SegmentStore> segmentStore = createSegmentStore(configuration);

	// create a valid request block
	boost::shared_ptr<Block> requestBlock = blockManager->blockAtLocation(request);

	// wrap requested block into Blocks
	pipeline::Value<Blocks> blocks;
	blocks->add(requestBlock);

	// create the SegmentGuarantor process node
	pipeline::Process< ::SegmentGuarantor> segmentGuarantor;

	segmentGuarantor->setInput("blocks", blocks);
	segmentGuarantor->setInput("slice store", sliceStore);
	segmentGuarantor->setInput("segment store", segmentStore);

	// let it do what it was build for
	//segmentGuarantor->guaranteeSegments();

	// TODO:
	// read output of missing slice blocks
	Locations missing;
	missing.push_back(util::point3<unsigned int>(1,2,3));
	missing.push_back(util::point3<unsigned int>(3,2,1));

	return missing;
}

} // namespace python
