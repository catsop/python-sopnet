#ifndef BLOCK_MANAGER_H__
#define BLOCK_MANAGER_H__

#include <boost/shared_ptr.hpp>

#include <sopnet/block/Block.h>
#include <sopnet/block/Box.h>
#include <util/point3.hpp>
#include <pipeline/Data.h>

using util::point3;

class Block;
class Blocks;

class BlockManager : public pipeline::Data
{
public:

    /**
	 * Creates a BlockManager for a stack with size stackSize in pixels and blocks of size
	 * blockSize, also in pixels.
	 */
    BlockManager(boost::shared_ptr<point3<unsigned int> > stackSize,
                 boost::shared_ptr<point3<unsigned int> > blockSize);

	/**
	 * Returns a shared_ptr to a Block at the given location, in pixels.
	 */
    virtual boost::shared_ptr<Block> blockAtLocation(unsigned int x, unsigned int y, unsigned int z);
	virtual boost::shared_ptr<Block> blockAtLocation(const boost::shared_ptr<point3<unsigned int> >& location);
    
	/**
	 * Returns a shared_ptr to a Block at the given block-offset from the given Block.
	 * @param block the block whose neighbor is desired
	 * @param offset the offset, in block-coordinates (not pixel coordinates).
	 * 
	 * For example, to get the Block just above the one in hand, one might call
	 * blockAtOffset(block, point3&ltint&gt::ptrTo(0, 0, 1));
	 */
    virtual boost::shared_ptr<Block> blockAtOffset(const Block& block,
										   const boost::shared_ptr<point3<int> >& offset);
	
	/**
	 * Returns a shared_ptr to a Block at the given block coordinates.
	 */
    virtual boost::shared_ptr<Block> blockAtCoordinates(const boost::shared_ptr<point3<unsigned int> >& coordinates) = 0;

	/**
	 * Returns the size of a block in pixels.
	 */
	virtual boost::shared_ptr<point3<unsigned int> > blockSize();

	/**
	 * Returns the size of the stack in pixels.
	 */
	virtual boost::shared_ptr<point3<unsigned int> > stackSize();
	
	/**
	 * Returns a Blocks containing all Block's overlapped by the given Box.
	 * @param box the box for which Blocks have been requested.
	 */
	virtual boost::shared_ptr<Blocks> blocksInBox(const boost::shared_ptr<Box<unsigned int> >& box);
	
protected:
    boost::shared_ptr<point3<unsigned int> > _stackSize, _blockSize;

};

#endif //BLOCK_MANAGER_H__

