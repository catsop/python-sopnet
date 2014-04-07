#include "BlockManager.h"
#include <util/Logger.h>
#include <boost/make_shared.hpp>
#include <sopnet/block/Box.h>
#include <sopnet/block/Blocks.h>

logger::LogChannel blockmanagerlog("blockmanagerlog", "[BlockManager] ");


/**
 * Creates an instance of a basic implementation of BlockManager, which exists only locally.
 */
BlockManager::BlockManager(const point3<unsigned int>& stackSize,
							const point3<unsigned int>& blockSize ) :
							_stackSize(stackSize), _blockSize(blockSize)
{
    _maxBlockCoordinates =
		(stackSize + blockSize - point3<unsigned int>(1u, 1u, 1u)) / blockSize;
	LOG_DEBUG(blockmanagerlog) << "Limit: " << stackSize << " / " << blockSize << " => " <<
		_maxBlockCoordinates << std::endl;
}

boost::shared_ptr<Block>
BlockManager::blockAtLocation(unsigned int x, unsigned int y, unsigned int z)
{
	return blockAtLocation(util::point3<unsigned int>(x, y, z));
}

boost::shared_ptr<Block>
BlockManager::blockAtLocation(const point3<unsigned int>& location)
{
    point3<unsigned int> blockCoordinates = location / _blockSize;
	return blockAtCoordinates(blockCoordinates);
}

boost::shared_ptr<Block>
BlockManager::blockAtOffset(const Block& block, const point3<int>& offset)
{
	point3<int> signedBlockCoordinates = offset + (block.location() / _blockSize);
	point3<unsigned int> blockCoordinates = signedBlockCoordinates;
	
	if (signedBlockCoordinates >= point3<int>(0,0,0) && blockCoordinates < _maxBlockCoordinates)
	{
		return blockAtCoordinates(blockCoordinates);
	}
	else
	{
		LOG_ALL(blockmanagerlog) << "Invalid block coordinates: " << offset << std::endl;
		LOG_ALL(blockmanagerlog) << "Max block coordinates: " << _maxBlockCoordinates << std::endl;
		return boost::shared_ptr<Block>();
	}
}

const util::point3<unsigned int>&
BlockManager::blockSize()
{
	return _blockSize;
}

const util::point3<unsigned int>&
BlockManager::stackSize()
{
	return _stackSize;
}

boost::shared_ptr<Blocks>
BlockManager::blocksInBox(const boost::shared_ptr< Box<unsigned int> >& box)
{
	util::point3<unsigned int> corner = box->location();
	util::point3<unsigned int> size = box->size();
	boost::shared_ptr<Blocks> blocks = boost::make_shared<Blocks>();
	for (unsigned int z = corner.z; z - corner.z < size.z; z += blockSize().z)
	{
		for (unsigned int y = corner.y; y - corner.y < size.y; y += blockSize().y)
		{
			for (unsigned int x = corner.x; x - corner.x < size.x; x += blockSize().x)
			{
				util::point3<unsigned int> coords = point3<unsigned int>(x, y, z) / blockSize();
				boost::shared_ptr<Block> block = blockAtCoordinates(coords);
				if (block)
				{
					blocks->add(block);
				}
			}
		}
	}

	return blocks;
}

const util::point3<unsigned int>&
BlockManager::maximumBlockCoordinates()
{
	return _maxBlockCoordinates;
}
