#include <boost/make_shared.hpp>

#include "LocalBlockManager.h"

LocalBlockManager::LocalBlockManager(boost::shared_ptr<point3<int> >stackSize,
									 boost::shared_ptr<point3<int> > blockSize): BlockManager(stackSize, blockSize),
									 _blockMap(boost::make_shared<PointBlockMap>())
{
	_lastId = 0;
}

boost::shared_ptr<Block>
LocalBlockManager::blockAtCoordinates(const boost::shared_ptr<point3<int> >& coordinates)
{
	//TODO: synchronize
	//point3<int> pt = *coordinates;

	if (_blockMap->count(*coordinates))
	{
		return (*_blockMap)[*coordinates];
	}
	else
	{
		// modular math, taking advantage of the modular properaties of int divide.
		boost::shared_ptr<point3<int> > corner = boost::make_shared<point3<int> >((*coordinates) * (*_blockSize));
		boost::shared_ptr<Block> block = boost::make_shared<Block>(_lastId++, coordinates, shared_from_this());
		
		(*_blockMap)[*corner] = block;
		
		return block;
	}
}


