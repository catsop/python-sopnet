#include "BlockManager.h"
#include <util/Logger.h>

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
    boost::shared_ptr<point3<unsigned int> > blockCoordinates = boost::make_shared<point3<unsigned int> >(*location / *_blockSize);
	return blockAtCoordinates(blockCoordinates);
}

boost::shared_ptr<Block>
BlockManager::blockAtOffset(const Block& block, const boost::shared_ptr<point3<int> >& offset)
{
	point3<int> signedBlockCoordinates = *offset + (*(block.location()) / *_blockSize);
	
	// TODO: check upper boundary, too.
	if (signedBlockCoordinates >= point3<int>(0,0,0))
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


