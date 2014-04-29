#include <boost/make_shared.hpp>

#include "LocalBlockManager.h"

LocalBlockManager::LocalBlockManager(const point3<unsigned int>& stackSize,
									 const point3<unsigned int>& blockSize,
									const point3<unsigned int>& coreSizeInBlocks):
									BlockManager(stackSize, blockSize, coreSizeInBlocks),
									 _blockMap(boost::make_shared<PointBlockMap>())
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
		if (_blockMap->count(coordinates))
		{
			return (*_blockMap)[coordinates];
		}
		else
		{
			// modular math, taking advantage of the modular properaties of int divide.
			point3<unsigned int> corner = coordinates * _blockSize;
			boost::shared_ptr<Block> block = boost::make_shared<Block>(_lastBlockId++, corner,
																	   shared_from_this());
			
			(*_blockMap)[coordinates] = block;
			
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
		boost::shared_ptr<Box<> > coreBox = boost::make_shared<Box<> >(coreLocation, _coreSize);
		boost::shared_ptr<Blocks> blocks = blocksInBox(coreBox);
		
		core = boost::shared_ptr<Core>();
		//boost::make_shared<Core>(_lastCoreId++, blocks, boost::shared_ptr<CoreManager>());
		
		_coreMap[coordinates] = core;
		
		return core;
	}
}


bool
LocalBlockManager::isValidZ(unsigned int z)
{
	return z < _stackSize.z;
}

bool
LocalBlockManager::isUpperBound(unsigned int z)
{
	return z == _stackSize.z - 1;
}

