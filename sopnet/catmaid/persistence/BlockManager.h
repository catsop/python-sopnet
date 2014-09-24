#ifndef BLOCK_MANAGER_H__
#define BLOCK_MANAGER_H__

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>

#include <catmaid/blocks/Block.h>
#include <util/Box.h>
#include <util/point3.hpp>
#include <pipeline/Data.h>

using util::point3;

class Core;
class Cores;
class Block;
class Blocks;

class BlockManager : public pipeline::Data
{
public:
	
	typedef boost::unordered_map<point3<unsigned int>, boost::shared_ptr<Block> > PointBlockMap;
	typedef boost::unordered_map<point3<unsigned int>, boost::shared_ptr<Core> > PointCoreMap;

    /**
	 * Creates a BlockManager for a stack with size stackSize in pixels and blocks of size
	 * blockSize, also in pixels.
	 */
    BlockManager(const point3<unsigned int> stackSize,
                 const point3<unsigned int> blockSize,
				 const point3<unsigned int> coreSizeInBlocks);

	/**
	 * Returns a shared_ptr to a Block at the given location, in pixels.
	 */
    virtual boost::shared_ptr<Block> blockAtLocation(unsigned int x, unsigned int y, unsigned int z);
	virtual boost::shared_ptr<Block> blockAtLocation(const point3<unsigned int>& location);
	
	/**
	 * Returns a shared_ptr to a Core at the given location, in pixels.
	 */
	virtual boost::shared_ptr<Core> coreAtLocation(unsigned int x, unsigned int y, unsigned int z);
	virtual boost::shared_ptr<Core> coreAtLocation(const util::point3<unsigned int> location);
    
	/**
	 * Returns a shared_ptr to a Block at the given block-offset from the given Block.
	 * @param block the block whose neighbor is desired
	 * @param offset the offset, in block-coordinates (not pixel coordinates).
	 * 
	 * For example, to get the Block just above the one in hand, one might call
	 * blockAtOffset(block, point3&ltint&gt::ptrTo(0, 0, 1));
	 */
    virtual boost::shared_ptr<Block> blockAtOffset(const Block& block,
										   const point3<int>& offset);
	
	/**
	 * Returns a shared_ptr to a Block at the given block coordinates.
	 */
    virtual boost::shared_ptr<Block> blockAtCoordinates(const point3<unsigned int>& coordinates) = 0;

	/**
	 * Returns a shared_ptr ot a Core at the given core coordinates.
	 */
	virtual boost::shared_ptr<Core> coreAtCoordinates(const util::point3<unsigned int> coordinates) = 0;
	
	virtual bool getSlicesFlag(boost::shared_ptr<Block> block) = 0;
	virtual bool getSegmentsFlag(boost::shared_ptr<Block> block) = 0;
	virtual bool getSolutionCostFlag(boost::shared_ptr<Block> block) = 0;
	virtual bool getSolutionSetFlag(boost::shared_ptr<Core> core) = 0;
	
	virtual void setSlicesFlag(boost::shared_ptr<Block> block, bool flag) = 0;
	virtual void setSegmentsFlag(boost::shared_ptr<Block> block, bool flag) = 0;
	virtual void setSolutionCostFlag(boost::shared_ptr<Block> block, bool flag) = 0;
	virtual void setSolutionSetFlag(boost::shared_ptr<Core> core, bool flag) = 0;
	
	/**
	 * Returns the size of a block in pixels.
	 */
	virtual const point3<unsigned int>& blockSize() const;
	
	/**
	 * Returns the size of a core in pixels.
	 */
	const util::point3<unsigned int>& coreSize();

	/**
	 * Returns the size of a core in blocks. 
	 */
	const util::point3<unsigned int>& coreSizeInBlocks();
	
	/**
	 * Returns the size of the stack in pixels.
	 */
	virtual const point3<unsigned int>& stackSize() const;
	
	/**
	 * Returns a Blocks containing all Block's overlapped by the given Box.
	 * @param box the box for which Blocks have been requested.
	 */
	virtual boost::shared_ptr<Blocks> blocksInBox(const Box<unsigned int>& box);

	/**
	 * Returns a Cores containing all Cores's overlapped by the given Box.
	 * @param box the box for which Cores have been requested.
	 */
	virtual boost::shared_ptr<Cores> coresInBox(const Box<>& box);
	
	/**
	 * Determines whether a z-coordinate represents a valid section
	 */
	virtual bool isValidZ(unsigned int z);
	
	/**
	 * Determines whether a given coordinate set will yeild a valid Block
	 */
	virtual bool isValidBlockCoordinates(const util::point3<unsigned int>& coords) const;

	/**
	 * Determines whether a given coordinate set will yeild a valid Core
	 */
	virtual bool isValidCoreCoordinates(const util::point3<unsigned int>& coords) const;
	
	/**
	 * Determines whether a given location will yeild a valid Block or Core
	 */
	virtual bool isValidLocation(const util::point3<unsigned int>& loc) const;
	
	
	/**
	 * Determines whether a z-coordinate represents the final section in the stack represented by this block manager.
	 */
	virtual bool isUpperBound(unsigned int z);
	
	virtual const point3<unsigned int>& maximumBlockCoordinates();
	
	virtual const point3<unsigned int>& maximumCoreCoordinates();
	
protected:
    point3<unsigned int> _stackSize, _blockSize, _coreSize, _coreSizeInBlocks;
	point3<unsigned int> _maxBlockCoordinates, _maxCoreCoordinates;

};

#endif //BLOCK_MANAGER_H__

