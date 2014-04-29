#include <boost/make_shared.hpp>

#include "LocalCoreManager.h"

LocalCoreManager::LocalCoreManager(const point3<unsigned int>& stackSize,
									 const point3<unsigned int>& blockSize,
									 const point3<unsigned int>& coreSizeInBlocks):
										CoreManager(stackSize, blockSize, coreSizeInBlocks),
									 _blockMap(boost::make_shared<PointBlockMap>())
{
	_lastId = 0;
}

boost::shared_ptr<Block>
LocalCoreManager::blockAtCoordinates(const point3<unsigned int>& coordinates)
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
			boost::shared_ptr<Block> block = boost::make_shared<Block>(_lastId++, corner,
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

bool
LocalCoreManager::isValidZ(unsigned int z)
{
	return z < _stackSize.z;
}

bool
LocalCoreManager::isUpperBound(unsigned int z)
{
	return z == _stackSize.z - 1;
}

