#include "LocalSliceStore.h"
#include <util/Logger.h>

logger::LogChannel localslicestorelog("localslicestorelog", "[LocalSliceStore] ");

void
LocalSliceStore::associateSlicesToBlock(
		const Slices& slices,
		const Block&  block,
		bool  doneWithBlock) {

	Slices& slicesForBlock = _slices[block];
	slicesForBlock.addAll(slices);

	if (doneWithBlock)
		_slicesFlags.insert(block);
}

void
LocalSliceStore::associateConflictSetsToBlock(
			const ConflictSets& conflictSets,
			const Block&        block) {

	ConflictSets& conflictsForBlock = _conflictSets[block];
	conflictsForBlock.addAll(conflictSets);
}

boost::shared_ptr<Slices>
LocalSliceStore::getSlicesByBlocks(
			const Blocks& blocks,
			Blocks&       missingBlocks) {

	boost::shared_ptr<Slices> _blocksSlices = boost::make_shared<Slices>();

	for (const Block& block : blocks) {
		std::map<Block, Slices>::const_iterator it = _slices.find(block);

		if (it == _slices.end())
			missingBlocks.add(block);
		else
			_blocksSlices->addAll(it->second);
	}

	return _blocksSlices;
}

boost::shared_ptr<ConflictSets>
LocalSliceStore::getConflictSetsByBlocks(
		const Blocks& blocks,
		Blocks&       missingBlocks) {

	boost::shared_ptr<ConflictSets> _blocksConflicts = boost::make_shared<ConflictSets>();

	for (const Block& block : blocks) {
		std::map<Block, ConflictSets>::const_iterator it = _conflictSets.find(block);

		if (it == _conflictSets.end())
			missingBlocks.add(block);
		else
			_blocksConflicts->addAll(it->second);
	}

	return _blocksConflicts;
}

bool
LocalSliceStore::getSlicesFlag(const Block& block) {

	return _slicesFlags.count(block) == 1;
}
