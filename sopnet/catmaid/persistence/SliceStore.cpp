#include "SliceStore.h"
#include <catmaid/persistence/SlicePointerHash.h>
#include <util/Logger.h>

logger::LogChannel slicestorelog("slicestorelog", "[SliceStore] ");

void
SliceStore::writeSlices(const Slices& slices, const ConflictSets& conflictSets, const Blocks& blocks)
{
	// TODO:
	// • move this into the slice guarantor: the slices guarantor should call 
	// associateSlicesToBlock for each block

	//boost::shared_ptr<ConflictSets> slicesConflictSets;
	//boost::shared_ptr<Slices> writtenSlices = boost::make_shared<Slices>();
	//SliceSet sliceSet;
	
	//foreach (boost::shared_ptr<Block> block, blocks)
	//{
		//boost::shared_ptr<Slices> blockSlices = collectSlicesByBlocks(slices, block);
		//associate(blockSlices, pipeline::Value<Block>(*block));
		//sliceSet.insert(blockSlices->begin(), blockSlices->end());
	//}

	//foreach (boost::shared_ptr<Slice> slice, sliceSet)
	//{
		//writtenSlices->add(slice);
	//}
	
	//slicesConflictSets = collectConflictBySlices(writtenSlices, conflictSets);

	//storeConflict(slicesConflictSets);
}

boost::shared_ptr<ConflictSets>
SliceStore::collectConflictBySlices(const boost::shared_ptr<Slices> slices, const ConflictSets& conflictSets)
{
	//boost::shared_ptr<ConflictSets> outConflictSets = boost::make_shared<ConflictSets>();
	
	//foreach (ConflictSet conflictSet, conflictSets)
	//{
		//if (containsAny(conflictSet, slices))
		//{
			//outConflictSets->add(conflictSet);

			//LOG_ALL(slicestorelog)
					//<< "Collected conflict of size "
					//<< conflictSet.getSlices().size()
					//<< std::endl;
		//}
	//}

	//return outConflictSets;
}

boost::shared_ptr<Slices>
SliceStore::collectSlicesByBlocks(const Slices& slices, const boost::shared_ptr<Block> block)
{
	//boost::shared_ptr<Slices> blockSlices = boost::make_shared<Slices>();
	//util::rect<unsigned int> blockRect = *block;

	//foreach (boost::shared_ptr<Slice> slice, slices)
	//{
		//if (blockRect.intersects(
			//static_cast<util::rect<unsigned int> >(slice->getComponent()->getBoundingBox())))
		//{
			//blockSlices->add(slice);
		//}
	//}

	//return blockSlices;
}

bool
SliceStore::containsAny(ConflictSet& conflictSet, const boost::shared_ptr<Slices> slices)
{
	//foreach (const boost::shared_ptr<Slice> slice, *slices)
	//{
		//if (conflictSet.getSlices().count(slice->getId()))
		//{
			//return true;
		//}
	//}
	
	//return false;
}

