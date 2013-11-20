#ifndef BLOCK_MANAGER_H__
#define BLOCK_MANAGER_H__

#include <boost/shared_ptr.hpp>

#include <sopnet/sopnet/block/Block.h>
#include <util/point3.hpp>

using util::point3;

class Block;

class BlockManager
{
public:

    /**
	 * Creates a BlockManager for a stack with size stackSize in pixels and blocks of size
	 * blockSize, also in pixels.
	 */
    BlockManager(boost::shared_ptr<point3<int> > stackSize,
                 boost::shared_ptr<point3<int> > blockSize);

	/**
	 * Returns a shared_ptr to a Block at the given location, in pixels.
	 */
    virtual boost::shared_ptr<Block> blockAtLocation(int x, int y, int z);
	virtual boost::shared_ptr<Block> blockAtLocation(const boost::shared_ptr<point3<int> >& location);
    
	/**
	 * Returns a shared_ptr to a Block at the given block-offset from the given Block.
	 * @param block the block whose neighbor is desired
	 * @param offset the offset, in block-coordinates (not pixel coordinates).
	 * 
	 * For example, to get the Block just above the one in hand, one might call
	 * blockAtOffset(block, point3&ltint&gt::ptrTo(0, 0, 1));
	 */
    virtual boost::shared_ptr<Block> blockAtOffset(const boost::shared_ptr<Block>& block,
										   const boost::shared_ptr<point3<int> >& offset);
	
	/**
	 * Returns a shared_ptr to a Block at the given block coordinates.
	 */
    virtual boost::shared_ptr<Block> blockAtCoordinates(const boost::shared_ptr<point3<int> >& coordinates) = 0;
	
protected:
    boost::shared_ptr<point3<int> > _stackSize, _blockSize;

};

#endif //BLOCK_MANAGER_H__

