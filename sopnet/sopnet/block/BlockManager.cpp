#include "BlockManager.h"
#include <util/Logger.h>
#include <boost/make_shared.hpp>
#include <sopnet/block/Box.h>
#include <sopnet/block/Blocks.h>

logger::LogChannel blockmanagerlog("blockmanagerlog", "[BlockManager] ");


/**
 * Creates an instance of a basic implementation of BlockManager, which exists only locally.
 */
BlockManager::BlockManager(boost::shared_ptr<point3<unsigned int> > stackSize,
							boost::shared_ptr<point3<unsigned int> > blockSize ) :
							_stackSize(stackSize), _blockSize(blockSize)
{
    
}

boost::shared_ptr<Block>
BlockManager::blockAtLocation(unsigned int x, unsigned int y, unsigned int z)
{
	return blockAtLocation(util::ptrTo(x, y, z));
}

boost::shared_ptr<Block>
BlockManager::blockAtLocation(const boost::shared_ptr<point3<unsigned int> >& location)
{
    boost::shared_ptr<point3<unsigned int> > blockCoordinates =
		boost::make_shared<point3<unsigned int> >(*location / *_blockSize);
	return blockAtCoordinates(blockCoordinates);
}

boost::shared_ptr<Block>
BlockManager::blockAtOffset(const Block& block, const boost::shared_ptr<point3<int> >& offset)
{
	point3<int> signedBlockCoordinates = *offset + (block.location() / *_blockSize);
	point3<unsigned int> maxBlockCoordinates = *stackSize() / *blockSize();
	
	if (signedBlockCoordinates >= point3<int>(0,0,0) && signedBlockCoordinates < maxBlockCoordinates)
	{
		boost::shared_ptr<point3<unsigned int> > blockCoordinates =
			boost::make_shared<point3<unsigned int> >(signedBlockCoordinates);
		return blockAtCoordinates(blockCoordinates);
	}
	else
	{
		return boost::shared_ptr<Block>();
	}
}

boost::shared_ptr<util::point3<unsigned int> >
BlockManager::blockSize()
{
	return _blockSize;
}

boost::shared_ptr<util::point3<unsigned int> >
BlockManager::stackSize()
{
	return _stackSize;
}

boost::shared_ptr<Blocks>
BlockManager::blocksInBox(const boost::shared_ptr< Box<unsigned int> >& box)
{
	util::point3<unsigned int> corner = box->location();
	util::point3<unsigned int> size = box->size();
	util::point3<unsigned int> currLoc = corner;
	boost::shared_ptr<Blocks> blocks = boost::make_shared<Blocks>();
	for (unsigned int z = corner.z; z - corner.z < size.z; z += blockSize()->z)
	{
		for (unsigned int y = corner.y; y - corner.y < size.y; y += blockSize()->y)
		{
			for (unsigned int x = corner.x; x - corner.x < size.x; x += blockSize()->x)
			{
				util::point3<unsigned int> coords = point3<unsigned int>(x, y, z) / *blockSize();
				boost::shared_ptr<Block> block = blockAtCoordinates(util::ptrTo(coords.x, coords.y, coords.z));
				blocks->add(block);
			}
		}
	}

	return blocks;
}


