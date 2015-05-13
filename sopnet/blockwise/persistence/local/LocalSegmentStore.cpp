#include "LocalSegmentStore.h"
#include <util/Logger.h>

logger::LogChannel localsegmentstorelog("localsegmentstorelog", "[LocalSegmentStore] ");

LocalSegmentStore::LocalSegmentStore(const ProjectConfiguration& config)
	: _weights(config.getLocalFeatureWeights()) {}

void
LocalSegmentStore::associateSegmentsToBlock(
		const SegmentDescriptions& segments,
		const Block&               block) {

	SegmentDescriptions& segmentsForBlock = _segments[block];
	for (const SegmentDescription& segment : segments)
		segmentsForBlock.add(segment);
}

boost::shared_ptr<SegmentDescriptions>
LocalSegmentStore::getSegmentsByBlocks(
		const Blocks& blocks,
		Blocks&       missingBlocks,
		bool          readCosts) { // readCosts is ignored.

	boost::shared_ptr<SegmentDescriptions> _blocksSegments = boost::make_shared<SegmentDescriptions>();

	for (const Block& block : blocks) {
		std::map<Block, SegmentDescriptions>::const_iterator it = _segments.find(block);

		if (it == _segments.end())
			missingBlocks.add(block);
		else
			for (const SegmentDescription& segment : it->second)
				_blocksSegments->add(segment);
	}

	return _blocksSegments;
}

boost::shared_ptr<SegmentConstraints>
LocalSegmentStore::getConstraintsByBlocks(
		const Blocks& blocks) {

	return boost::make_shared<SegmentConstraints>();
}

std::vector<double>
LocalSegmentStore::getFeatureWeights() {

	return _weights;
}

void
LocalSegmentStore::storeSegmentCosts(
		const std::map<SegmentHash, double>& costs) {

	UTIL_THROW_EXCEPTION(
			NotYetImplemented,
			"LocalSegmentStore does not yet store segment costs.")
}

void
LocalSegmentStore::storeSolution(
		const std::vector<std::set<SegmentHash> >& assemblies,
		const Core&                                core) {

	_solutions[core] = assemblies;
}

bool
LocalSegmentStore::getSegmentsFlag(
		const Block& block) {

	return _segments.find(block) != _segments.end();
}
