#include "LocalSliceStore.h"
#include <boost/make_shared.hpp>
#include <set>
#include <utility>

#include <sopnet/slices/ConflictSet.h>
#include <sopnet/slices/ConflictSets.h>

#include <util/Logger.h>
logger::LogChannel localslicestorelog("localslicestorelog", "[LocalSliceStore] ");

LocalSliceStore::LocalSliceStore()
{
}


pipeline::Value<Blocks>
LocalSliceStore::getAssociatedBlocks(pipeline::Value<Slice> slice)
{
	if (_sliceBlockMap.count(*slice))
	{
		return _sliceBlockMap[*slice];
	}
	else
	{
		return pipeline::Value<Blocks>();
	}
}

pipeline::Value<Slices>
LocalSliceStore::retrieveSlices(const Blocks& blocks)
{
	pipeline::Value<Slices> slices = pipeline::Value<Slices>();
	// Use a set to ensure that we don't accidentally push the same Slice multiple times into to
	// the returned Slices.
	SliceSet blockSliceSet;
	SliceSet conflictSliceSet;
	boost::unordered_set<ConflictSet> conflictSetUSet;
	
	// Retrieve the slices that belong to the requested blocks
	foreach (boost::shared_ptr<Block> block, blocks)
	{
	if (_blockSliceMap.count(*block))
		{
			LOG_ALL(localslicestorelog) << "Found block " << *block << " in block slice map" <<
				std::endl;
			pipeline::Value<Slices> blockSlices = _blockSliceMap[*block];
			blockSliceSet.insert(blockSlices->begin(), blockSlices->end());
			LOG_ALL(localslicestorelog) << "Retrieved " << blockSlices->size() <<
				" slices from block" << std::endl;
		}
	}
	
	LOG_ALL(localslicestorelog) << "Retrieved " << blockSliceSet.size() << " slices in total" << std::endl;
	
	// Retrieve all slices that are in conflict sets with the slices we've already retrieved.
	foreach (boost::shared_ptr<Slice> blockSlice, blockSliceSet)
	{
		// First, push all ConflictSets into an actual unordered_set. Otherwise, we might
		// try to add the slices from each conflict set n times, where n is the size of the
		// conflictSet.
		pipeline::Value<ConflictSets> conflictSets = _conflictMap[blockSlice->getId()];
		conflictSetUSet.insert(conflictSets->begin(), conflictSets->end());
	}
	
	foreach (ConflictSet conflictSet, conflictSetUSet)
	{
		// For each id in the conflict set, find the Slice with the given id and add it to the 
		// conflictSliceSet.
		foreach (unsigned int id, conflictSet.getSlices())
		{
			conflictSliceSet.insert(_idSliceMap[id]);
		}
	}
	
	// Merge the conflict set into the  block slice set
	blockSliceSet.insert(conflictSliceSet.begin(), conflictSliceSet.end());

	foreach (boost::shared_ptr<Slice> slice, blockSliceSet)
	{
		slices->add(slice);
	}
	
	// Set the conflict information for each slices
	foreach (ConflictSet conflictSet, conflictSetUSet)
	{
		slices->addConflicts(conflictSet.getSlices());
	}
	
	return slices;
}

void
LocalSliceStore::mapBlockToSlice(const boost::shared_ptr< Block > block, const boost::shared_ptr< Slice > slice)
{
	// Place entry in block slice map

	pipeline::Value<Slices> slices = _blockSliceMap[*block];
	
	foreach (boost::shared_ptr<Slice> cSlice, *slices)
	{
		if (*cSlice == *slice)
		{
			LOG_ALL(localslicestorelog) << "BlockSliceMap already links Block " <<
				block->getId() << " to slice " << slice->getId() << std::endl;
			return;
		}
	}
	
	slices->add(slice);
}

void
LocalSliceStore::mapSliceToBlock(const boost::shared_ptr< Slice > slice, const boost::shared_ptr< Block > block)
{
	// Place entry in slice block map
	pipeline::Value<Blocks> blocks = _sliceBlockMap[*slice];
	
	foreach (boost::shared_ptr<Block> cBlock, *blocks)
	{
		if (*block == *cBlock)
		{
			LOG_ALL(localslicestorelog) << "SliceBlockMap already links slice " <<
				slice->getId() << " to block " << block->getId() << std::endl;
			return;
		}
		
	}
	
	blocks->add(block);
}

void
LocalSliceStore::associate(pipeline::Value<Slices> slicesIn,
							pipeline::Value<Block> block)
{
	foreach (boost::shared_ptr<Slice> slice, *slicesIn)
	{
		// Check to see if we've already stored this Slice, in the sense that we stored a different
		// Slice object that contains the same geometry. If so, eqSlice will point to the old one.
		// If not, the eqSlice pointer will be equal to the slice pointer.
		boost::shared_ptr<Slice> eqSlice = equivalentSlice(slice);

		// Map the old pointer
		mapBlockToSlice(block, eqSlice);
		mapSliceToBlock(eqSlice, block);

		if (_sliceMasterSet.count(slice))
		{
			// If we have already stored this Slice, map the new id to the old object.
			unsigned int existingId = (*_sliceMasterSet.find(slice))->getId();
			_idSliceMap[slice->getId()] = _idSliceMap[existingId];
		}
		else
		{
			// If we have not already stored this slice, map it to its id, and insert it into
			// the set.
			_idSliceMap[slice->getId()] = slice;
			_sliceMasterSet.insert(slice);
		}
	}
}

boost::shared_ptr<Slice>
LocalSliceStore::equivalentSlice(const boost::shared_ptr<Slice> slice)
{
	if (_sliceMasterSet.count(slice))
	{
		
		return *_sliceMasterSet.find(slice);
	}
	else
	{
		return slice;
	}
}

void
LocalSliceStore::storeConflict(pipeline::Value<ConflictSets> conflictSets)
{
	
	
	foreach (const ConflictSet conflict, *conflictSets)
	{
		ConflictSet eqConflict;
		foreach (const unsigned int id, conflict.getSlices())
		{
			if (_idSliceMap.count(id))
			{
				eqConflict.addSlice(_idSliceMap[id]->getId());
			}
			else
			{
				LOG_ALL(localslicestorelog) << "Missing slices while storing conflict: " << id <<
					std::endl;
			}
			
		}
		
		LOG_ALL(localslicestorelog) << "     input set: " << conflict << std::endl;
		LOG_ALL(localslicestorelog) << "storing eq set: " << eqConflict << std::endl << std::endl;
		

		foreach (const unsigned int id, eqConflict.getSlices())
		{
			_conflictMap[id]->add(eqConflict);
		}

	}
}

pipeline::Value<ConflictSets>
LocalSliceStore::retrieveConflictSets(pipeline::Value<Slices> slices)
{
	pipeline::Value<ConflictSets> allConflictSets;
	boost::unordered_set<ConflictSet> conflictSetUSet;
	
	// Oh, man. This could be really expensive. We're only doing it for testing, so it should be ok
	foreach (boost::shared_ptr<Slice> slice, *slices)
	{
		boost::shared_ptr<Slice> eqSlice = equivalentSlice(slice);
		pipeline::Value<ConflictSets> conflictSets = _conflictMap[eqSlice->getId()];
		conflictSetUSet.insert(conflictSets->begin(), conflictSets->end());
	}
	
	foreach (ConflictSet conflictSet, conflictSetUSet)
	{
		allConflictSets->add(conflictSet);
		LOG_ALL(localslicestorelog) << "Retrieved conflict: " << conflictSet << std::endl;
	}
	
	return allConflictSets;
}


void
LocalSliceStore::dumpStore()
{
	SliceBlockMap::iterator sbm_it;
	BlockSliceMap::iterator bsm_it;
	
	LOG_DEBUG(localslicestorelog) << "I have " << _sliceMasterSet.size() << " slices recorded" <<
		std::endl;
	
	foreach (const boost::shared_ptr<Slice> slice, _sliceMasterSet)
	{
		LOG_DEBUG(localslicestorelog) << "Slice id: " << slice->getId() << "\tHash: " <<
			slice->hashValue() << std::endl;
	}
	
	for (bsm_it = _blockSliceMap.begin(); bsm_it != _blockSliceMap.end(); ++bsm_it)
	{
		LOG_DEBUG(localslicestorelog) << "Block " << bsm_it->first.getId() << " " << bsm_it->first << std::endl;
	}
	
	
	for (sbm_it = _sliceBlockMap.begin(); sbm_it != _sliceBlockMap.end(); ++sbm_it)
	{
		LOG_DEBUG(localslicestorelog) << "Slice id: " << sbm_it->first.getId() <<
			" with blocks";
		foreach (boost::shared_ptr<Block> block, *sbm_it->second)
		{
			  LOG_DEBUG(localslicestorelog) << " " << block->getId();
		}
		LOG_DEBUG(localslicestorelog) << std::endl;
	}
	
	for (bsm_it = _blockSliceMap.begin(); bsm_it != _blockSliceMap.end(); ++bsm_it)
	{
		LOG_DEBUG(localslicestorelog) << "Block " <<  bsm_it->first.getId() << " with  slices";
		foreach (boost::shared_ptr<Slice> slice, *bsm_it->second)
		{
			LOG_DEBUG(localslicestorelog) << " " << slice->getId();
		}
		LOG_DEBUG(localslicestorelog) << std::endl;
	}
}
