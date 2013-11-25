#include "BlockManager.h"

/**
 * Creates an instance of a basic implementation of BlockManager, which exists only locally.
 */
BlockManager::BlockManager(boost::shared_ptr<point3<int> > stackSize,
							boost::shared_ptr<point3<int> > blockSize ) :
							_stackSize(stackSize), _blockSize(blockSize)
{
    
}

boost::shared_ptr<Block>
BlockManager::blockAtLocation(int x, int y, int z)
{
	return blockAtLocation(util::ptrTo(x, y, z));
}

boost::shared_ptr<Block>
BlockManager::blockAtLocation(const boost::shared_ptr<point3<int> >& location)
{
    boost::shared_ptr<point3<int> > blockCoordinates = boost::make_shared<point3<int> >(*location / *_blockSize);
	return blockAtCoordinates(blockCoordinates);
}

boost::shared_ptr<Block>
BlockManager::blockAtOffset(const Block& block, const boost::shared_ptr<point3<int> >& offset)
{
    boost::shared_ptr<point3<int> > blockCoordinates =
		boost::make_shared<point3<int> >((*(block.location()) / *_blockSize) + *offset);
	return blockAtCoordinates(blockCoordinates);
}
