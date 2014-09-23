#include <boost/make_shared.hpp>

#include "LocalBlockManager.h"

LocalBlockManager::LocalBlockManager(const point3<unsigned int>& stackSize,
									 const point3<unsigned int>& blockSize,
									const point3<unsigned int>& coreSizeInBlocks):
									BlockManager(stackSize, blockSize, coreSizeInBlocks)
{
	_lastBlockId = 0;
	_lastCoreId = 0;
}

boost::shared_ptr<Block>
LocalBlockManager::blockAtCoordinates(const point3<unsigned int>& coordinates)
{
	//TODO: synchronize
	//point3<int> pt = *coordinates;
	
	if (coordinates < _maxBlockCoordinates)
	{
		if (_blockMap.count(coordinates))
		{
			return _blockMap[coordinates];
		}
		else
		{
			// modular math, taking advantage of the modular properaties of int divide.
			point3<unsigned int> corner = coordinates * _blockSize;
			boost::shared_ptr<Block> block = boost::make_shared<Block>(_lastBlockId++, corner,
																	   shared_from_this());
			
			_blockSlicesFlagMap[*block] = false;
			_blockSegmentsFlagMap[*block] = false;
			_blockSolutionCostFlagMap[*block] = false;
			
			_blockMap[coordinates] = block;
			
			return block;
		}
	}
	else
	{
		return boost::shared_ptr<Block>();
	}
}

boost::shared_ptr<Core>
LocalBlockManager::coreAtCoordinates(const util::point3<unsigned int> coordinates)
{
	if (_coreMap.count(coordinates))
	{
		return _coreMap[coordinates];
	}
	else
	{
		boost::shared_ptr<Core> core;
		util::point3<unsigned int> coreLocation = coordinates * _coreSize;
		Box<> coreBox(coreLocation, _coreSize);
		boost::shared_ptr<Blocks> blocks = blocksInBox(coreBox);
		
		core = boost::make_shared<Core>(_lastCoreId++, blocks);
		_coreSolutionSetFlagMap[*core] = false;
		
		_coreMap[coordinates] = core;
		
		return core;
	}
}

bool
LocalBlockManager::getSegmentsFlag(boost::shared_ptr<Block> block)
{
	return getFlag(*block, _blockSegmentsFlagMap);
}

void
LocalBlockManager::setSegmentsFlag(boost::shared_ptr<Block> block, bool flag)
{
	_blockSegmentsFlagMap[*block] = flag;
}

bool
LocalBlockManager::getSlicesFlag(boost::shared_ptr<Block> block)
{
	return getFlag(*block, _blockSlicesFlagMap);
}

void
LocalBlockManager::setSlicesFlag(boost::shared_ptr<Block> block, bool flag)
{
	_blockSlicesFlagMap[*block] = flag;
}

bool
LocalBlockManager::getSolutionCostFlag(boost::shared_ptr<Block> block)
{
	return getFlag(*block, _blockSolutionCostFlagMap);
}

void
LocalBlockManager::setSolutionCostFlag(boost::shared_ptr<Block> block, bool flag)
{
	_blockSolutionCostFlagMap[*block] = flag;
}

bool
LocalBlockManager::getSolutionSetFlag(boost::shared_ptr<Core> core)
{
	if (_coreSolutionSetFlagMap.count(*core))
	{
		return _coreSolutionSetFlagMap[*core];
	}
	else
	{
		return false;
	}
}

void
LocalBlockManager::setSolutionSetFlag(boost::shared_ptr<Core> core, bool flag)
{
	_coreSolutionSetFlagMap[*core] = flag;
}

bool
LocalBlockManager::getFlag(const Block& block, boost::unordered::unordered_map<Block, bool> map)
{
	if (map.count(block))
	{
		return map[block];
	}
	else
	{
		return false;
	}
}

