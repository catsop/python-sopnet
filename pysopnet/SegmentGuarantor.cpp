#include <catmaid/guarantors/SegmentGuarantor.h>
#include <catmaid/blocks/Blocks.h>
#include <util/point.hpp>
#include "SegmentGuarantor.h"
#include "logging.h"

namespace python {

Locations
SegmentGuarantor::fill(
		const util::point<unsigned int, 3>& request,
		const SegmentGuarantorParameters& /*parameters*/,
		const ProjectConfiguration& configuration) {

	LOG_USER(pylog) << "[SegmentGuarantor] fill called for block at " << request << std::endl;

	boost::shared_ptr<StackStore>   rawStackStore = createStackStore(configuration, Raw);
	//boost::shared_ptr<BlockManager> blockManager = createBlockManager(configuration);
	boost::shared_ptr<SliceStore>   sliceStore   = createSliceStore(configuration);
	boost::shared_ptr<SegmentStore> segmentStore = createSegmentStore(configuration);

	// create a valid request block
	Block requestBlock(request.x(), request.y(), request.z());

	// wrap requested block into Blocks
	Blocks blocks;
	blocks.add(requestBlock);

	// create a SegmentGuarantor
	::SegmentGuarantor segmentGuarantor(
			configuration,
			segmentStore,
			sliceStore,
			rawStackStore);

	// let it do what it was build for
	Blocks missingBlocks = segmentGuarantor.guaranteeSegments(blocks);

	// collect missing block locations
	Locations missing;
	foreach (const Block& block, missingBlocks)
		missing.push_back(util::point<unsigned int, 3>(block.x(), block.y(), block.z()));

	return missing;
}

} // namespace python
