#include "BlockManager.h"

BlockManager::BlockManager(boost::shared_ptr<Point3<int> > stackSize,
							boost::shared_ptr<Point3<int> > blockSize ) :
							_stackSize(stackSize), _blockSize(blockSize)
{
    
}

