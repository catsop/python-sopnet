#ifndef BLOCK_MANAGER_H__
#define BLOCK_MANAGER_H__

#include <boost/shared_ptr.hpp>

#include <sopnet/sopnet/block/Block.h>
#include <sopnet/sopnet/block/Point3.h>

class Block;

class BlockManager
{
public:

    // width - x dimension, height - y dimension, depth - z dimension
    BlockManager(boost::shared_ptr<Point3<int> > stackSize,
                 boost::shared_ptr<Point3<int> > blockSize);

    virtual boost::shared_ptr<Block> blockAtLocation(int x, int y, int z);
	virtual boost::shared_ptr<Block> blockAtLocation(const boost::shared_ptr<Point3<int> >& location);
    
    virtual boost::shared_ptr<Block> blockAtOffset(const boost::shared_ptr<Block>& block,
										   const boost::shared_ptr<Point3<int> >& offset);
    
protected:
    boost::shared_ptr<Point3<int> > _stackSize, _blockSize;
};

#endif //BLOCK_MANAGER_H__

