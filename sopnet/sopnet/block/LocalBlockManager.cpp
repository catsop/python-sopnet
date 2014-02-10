#include <boost/make_shared.hpp>

#include "LocalBlockManager.h"

LocalBlockManager::LocalBlockManager(const point3<unsigned int>& stackSize,
									 const point3<unsigned int>& blockSize): BlockManager(stackSize, blockSize),
									 _blockMap(boost::make_shared<PointBlockMap>())
{
	_lastId = 0;
}

boost::shared_ptr<Block>
LocalBlockManager::blockAtCoordinates(const point3<unsigned int>& coordinates)
{
	//TODO: synchronize
	//point3<int> pt = *coordinates;

	if (_blockMap->count(coordinates))
	{
		return (*_blockMap)[coordinates];
	}
	else
	{
		// modular math, taking advantage of the modular properaties of int divide.
		point3<unsigned int> corner = coordinates * _blockSize;
		boost::shared_ptr<Block> block = boost::make_shared<Block>(_lastId++, corner, shared_from_this());
		
		(*_blockMap)[coordinates] = block;
		
		return block;
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

