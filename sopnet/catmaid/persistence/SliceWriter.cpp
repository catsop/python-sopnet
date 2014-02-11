#include "SliceWriter.h"

#include <boost/unordered_set.hpp>
#include <boost/shared_ptr.hpp>
#include <sopnet/slices/Slice.h>
#include <util/foreach.h>

SliceWriter::SliceWriter()
{
	registerInput(_blocks, "blocks");
	registerInput(_slices, "slices");
	registerInput(_store, "store");
	registerInput(_conflictSets, "conflict sets");
}

void
SliceWriter::writeSlices()
{
	updateInputs();
	
	foreach (boost::shared_ptr<Block> block, *_blocks)
	{
		pipeline::Value<Slices> blockSlices = collectSlicesByBlocks(block);
		pipeline::Value<ConflictSets> slicesConflictSets = collectConflictBySlices(blockSlices);

		_store->associate(blockSlices, pipeline::Value<Block>(*block));
		_store->storeConflict(slicesConflictSets);
	}
}

bool
SliceWriter::containsAny(ConflictSet& conflictSet, pipeline::Value<Slices>& slices)
{
	foreach (const boost::shared_ptr<Slice> slice, *slices)
	{
		if (conflictSet.getSlices().count(slice->getId()))
		{
			return true;
		}
	}
	
	return false;
}


pipeline::Value<ConflictSets>
SliceWriter::collectConflictBySlices(pipeline::Value<Slices> slices)
{
	boost::unordered_set<ConflictSet> conflictSetSet;
	pipeline::Value<ConflictSets> conflictSets;
	
	foreach (ConflictSet conflictSet, *_conflictSets)
	{
		if (containsAny(conflictSet, slices))
		{
			conflictSetSet.insert(conflictSet);
		}
	}
	
	foreach (const ConflictSet conflictSet, conflictSetSet)
	{
		conflictSets->add(conflictSet);
	}
	
	return conflictSets;
}

pipeline::Value<Slices>
SliceWriter::collectSlicesByBlocks(const boost::shared_ptr<Block> block)
{
	pipeline::Value<Slices> blockSlices;

	foreach (boost::shared_ptr<Slice> slice, *_slices)
	{
		if (block->overlaps(slice->getComponent()))
		{
			blockSlices->add(slice);
		}
	}

	return blockSlices;
}
