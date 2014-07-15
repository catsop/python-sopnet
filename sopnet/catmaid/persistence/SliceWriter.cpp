#include "SliceWriter.h"

#include <boost/unordered_set.hpp>
#include <boost/shared_ptr.hpp>
#include <sopnet/slices/Slice.h>
#include <util/foreach.h>
#include <catmaid/persistence/SlicePointerHash.h>
#include <util/Logger.h>

logger::LogChannel slicewriterlog("slicewriterlog", "[SliceWriter] ");

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
	boost::shared_ptr<ConflictSets> slicesConflictSets;
	boost::shared_ptr<Slices> writtenSlices = boost::make_shared<Slices>();
	SliceSet sliceSet;
	updateInputs();
	
	foreach (boost::shared_ptr<Block> block, *_blocks)
	{
		boost::shared_ptr<Slices> blockSlices = collectSlicesByBlocks(block);
		_store->associate(blockSlices, pipeline::Value<Block>(*block));
		sliceSet.insert(blockSlices->begin(), blockSlices->end());
	}

	foreach (boost::shared_ptr<Slice> slice, sliceSet)
	{
		writtenSlices->add(slice);
	}
	
	slicesConflictSets = collectConflictBySlices(writtenSlices);

	_store->storeConflict(slicesConflictSets);
}


bool
SliceWriter::containsAny(ConflictSet& conflictSet, const boost::shared_ptr<Slices> slices)
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


boost::shared_ptr<ConflictSets>
SliceWriter::collectConflictBySlices(boost::shared_ptr<Slices> slices)
{
	boost::shared_ptr<ConflictSets> conflictSets = boost::make_shared<ConflictSets>();
	
	foreach (ConflictSet conflictSet, *_conflictSets)
	{
		if (containsAny(conflictSet, slices))
		{
			conflictSets->add(conflictSet);

			LOG_ALL(slicewriterlog)
					<< "Collected conflict of size "
					<< conflictSet.getSlices().size()
					<< std::endl;
		}
	}

	return conflictSets;
}

boost::shared_ptr<Slices>
SliceWriter::collectSlicesByBlocks(const boost::shared_ptr<Block> block)
{
	boost::shared_ptr<Slices> blockSlices = boost::make_shared<Slices>();
	util::rect<unsigned int> blockRect = *block;

	foreach (boost::shared_ptr<Slice> slice, *_slices)
	{
		if (blockRect.intersects(
			static_cast<util::rect<unsigned int> >(slice->getComponent()->getBoundingBox())))
		{
			blockSlices->add(slice);
		}
	}

	return blockSlices;
}
