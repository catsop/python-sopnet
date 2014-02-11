#include "LocalSliceStore.h"
#include <boost/make_shared.hpp>
#include <set>
#include <utility>

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
LocalSliceStore::retrieveSlices(pipeline::Value<Blocks> blocks)
{
	pipeline::Value<Slices> slices = pipeline::Value<Slices>();
	SliceSet blockSliceSet;
	
	foreach (boost::shared_ptr<Block> block, *blocks)
	{
	if (_blockSliceMap.count(*block))
		{
			LOG_ALL(localslicestorelog) << "Found block " << *block << " in block slice map" <<
				std::endl;
			pipeline::Value<Slices> blockSlices = _blockSliceMap[*block];
			blockSliceSet.insert(blockSlices->begin(), blockSlices->end());
		}
	}

	foreach (boost::shared_ptr<Slice> slice, blockSliceSet)
	{
		slices->add(slice);
	}
	
	return slices;
}

void
LocalSliceStore::mapBlockToSlice(const boost::shared_ptr< Block >& block, const boost::shared_ptr< Slice >& slice)
{
	// Place entry in block slice map
	pipeline::Value<Slices> slices = _blockSliceMap[*block];
	
// 	if (_blockSliceMap.count(*block))
// 	{
// 		
// 	}
// 	else
// 	{
// 		slices = boost::make_shared<Slices>();
// 		_blockSliceMap[*block] = slices;
// 	}
	
	foreach (boost::shared_ptr<Slice> cSlice, *slices)
	{
		if (*cSlice == *slice)
		{
			LOG_DEBUG(localslicestorelog) << "BlockSliceMap already links Block " <<
				block->getId() << " to slice " << slice->getId() << std::endl;
			return;
		}
	}
	
	slices->add(slice);
}

void
LocalSliceStore::mapSliceToBlock(const boost::shared_ptr< Slice >& slice, const boost::shared_ptr< Block >& block)
{
	// Place entry in slice block map
	pipeline::Value<Blocks> blocks = _sliceBlockMap[*slice];
	
	
// 	if (_sliceBlockMap->count(*slice))
// 	{
// 		blocks = (*_sliceBlockMap)[*slice];
// 	}
// 	else
// 	{
// 		blocks = boost::make_shared<Blocks>();
// 		(*_sliceBlockMap)[*slice] = blocks;
// 	}
	
	foreach (boost::shared_ptr<Block> cBlock, *blocks)
	{
		if (*block == *cBlock)
		{
			LOG_DEBUG(localslicestorelog) << "SliceBlockMap already links slice " <<
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
			// If we have not already stored this slice, map it to its id, and sert it into
			// the set.
			_idSliceMap[slice->getId()] = slice;
			_sliceMasterSet.insert(slice);
		}
	}
}

boost::shared_ptr<Slice>
LocalSliceStore::equivalentSlice(const boost::shared_ptr<Slice>& slice)
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
	boost::unordered_set<ConflictSet> conflictSetSet;
	
	// Oh, man. This could be really expensive. We're only doing it for testing, so it should be ok
	foreach (boost::shared_ptr<Slice> slice, *slices)
	{
		boost::shared_ptr<Slice> eqSlice = equivalentSlice(slice);
		pipeline::Value<ConflictSets> conflictSets = _conflictMap[eqSlice->getId()];
		conflictSetSet.insert(conflictSets->begin(), conflictSets->end());
	}
	
	foreach (ConflictSet conflictSet, conflictSetSet)
	{
		allConflictSets->add(conflictSet);
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
	
	for (sbm_it = _sliceBlockMap.begin(); sbm_it != _sliceBlockMap.end(); ++sbm_it)
	{
		LOG_DEBUG(localslicestorelog) << "Slice id: " << sbm_it->first.getId() <<
			" with " << sbm_it->second->length() << " blocks " << std::endl;
	}
	
	for (bsm_it = _blockSliceMap.begin(); bsm_it != _blockSliceMap.end(); ++bsm_it)
	{
		LOG_DEBUG(localslicestorelog) << "Block " << bsm_it->first << " with " <<
			bsm_it->second->size() << " slices" << std::endl;
	}
}
