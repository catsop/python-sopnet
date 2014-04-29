#ifndef CORE_MANAGER_H__
#define CORE_MANAGER_H__

#include <boost/shared_ptr.hpp>

#include <sopnet/block/Box.h>
#include <sopnet/block/Core.h>
#include <sopnet/block/Cores.h>
#include <pipeline/all.h>
#include <util/point3.hpp>
#include <boost/unordered_map.hpp>
#include <boost/enable_shared_from_this.hpp>

using util::point3;

class Core;
class Cores;
class Block;
class Blocks;

class CoreManager : public pipeline::Data, public boost::enable_shared_from_this<CoreManager>
{
public:
	CoreManager(const point3<unsigned int>& stackSize,
				const point3<unsigned int>& blockSize,
				const point3<unsigned int>& coreSizeInBlocks);
	
	boost::shared_ptr<Core> coreAtLocation(unsigned int x, unsigned int y, unsigned int z);
	boost::shared_ptr<Core> coreAtLocation(const util::point3<unsigned int> location);
	boost::shared_ptr<Core> coreAtCoordinates(const util::point3<unsigned int> coordinates);
	
	boost::shared_ptr<Cores> coresInBox(const boost::shared_ptr<Box<> > box);
	
	const util::point3<unsigned int>& coreSize();
	
	const util::point3<unsigned int>& coreSizeInBlocks();
	
	/**
	 * Determines whether a given coordinate set will yeild a valid Core
	 */
	virtual bool isValidCoreCoordinates(const util::point3<unsigned int>& coords) const;
	
	/**
	 * Determines whether a given location will yeild a valid Core or Block
	 */
	virtual bool isValidLocation(const util::point3<unsigned int>& loc) const;
	
	/**
	 * Determines whether a given coordinate set will yeild a valid Block
	 */
	virtual bool isValidBlockCoordinates(const util::point3<unsigned int>& coords) const;
	
	/**
	 * Returns a shared_ptr to a Block at the given location, in pixels.
	 */
    virtual boost::shared_ptr<Block> blockAtLocation(unsigned int x, unsigned int y, unsigned int z);
	virtual boost::shared_ptr<Block> blockAtLocation(const point3<unsigned int>& location);
    
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
	 * Returns the size of a block in pixels.
	 */
	virtual const point3<unsigned int>& blockSize() const;

	/**
	 * Returns the size of the stack in pixels.
	 */
	virtual const point3<unsigned int>& stackSize() const;
	
	/**
	 * Returns a Blocks containing all Block's overlapped by the given Box.
	 * @param box the box for which Blocks have been requested.
	 */
	virtual boost::shared_ptr<Blocks> blocksInBox(const boost::shared_ptr<Box<unsigned int> >& box);
	
	/**
	 * Determines whether a z-coordinate represents a valid section
	 */
	virtual bool isValidZ(unsigned int z) = 0;
	
	/**
	 * Determines whether a z-coordinate represents the final section in the stack represented by this block manager.
	 */
	virtual bool isUpperBound(unsigned int z) = 0;
	
	virtual const point3<unsigned int>& maximumBlockCoordinates();
	
protected:
    point3<unsigned int> _stackSize, _blockSize;
	point3<unsigned int> _maxBlockCoordinates;

	
private:
	boost::unordered_map<util::point3<unsigned int>, boost::shared_ptr<Core> > _coreMap;
	boost::shared_ptr<CoreManager> _blockManager;
	util::point3<unsigned int> _coreSize, _coreSizeInBlocks, _maxCoreCoordinates;
	unsigned int _lastId;
};

#endif //CORE_MANAGER_H__