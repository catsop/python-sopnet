#include "Block.h"
#include <boost/concept_check.hpp>
#include <util/Logger.h>
#include <catmaid/persistence/BlockManager.h>

logger::LogChannel blocklog("blocklog", "[Block] ");

util::point3<unsigned int>
Block::blockSize(const boost::shared_ptr<BlockManager>& blockManager,
				 const util::point3<unsigned int>& location)
{
	point3<unsigned int> maxSize = blockManager->stackSize() - location;
	point3<unsigned int> size = blockManager->blockSize().min(maxSize);
	return size;
}


Block::Block()
{
	//invalid block
}

Block::Block(unsigned int id, const point3<unsigned int>& loc,
			boost::shared_ptr<BlockManager> manager) : Box<>(loc, blockSize(manager, loc)),
			_manager(manager), _id(id)
{
    
}

unsigned int
Block::getId() const
{
    return _id;
}

boost::shared_ptr<BlockManager>
Block::getManager() const
{
	return _manager;
}

void
Block::setSlicesFlag(bool flag)
{
	boost::shared_ptr<Block> thisPtr = shared_from_this();
	_manager->setSlicesFlag(thisPtr, flag);
}

void
Block::setSegmentsFlag(bool flag)
{
	_manager->setSegmentsFlag(shared_from_this(), flag);
}

void
Block::setSolutionCostFlag(bool flag)
{
	boost::shared_ptr<Block> thisPtr = shared_from_this();
	_manager->setSolutionCostFlag(thisPtr, flag);
}

bool
Block::getSegmentsFlag()
{
	boost::shared_ptr<Block> thisPtr = shared_from_this();
	return _manager->getSegmentsFlag(thisPtr);
}

bool
Block::getSlicesFlag()
{
	boost::shared_ptr<Block> thisPtr = shared_from_this();
	return _manager->getSlicesFlag(thisPtr);
}

bool Block::getSolutionCostFlag()
{
	boost::shared_ptr<Block> thisPtr = shared_from_this();
	return _manager->getSolutionCostFlag(thisPtr);
}

util::point3<unsigned int>
Block::getCoordinates() const
{
	return _location / _manager->blockSize();
}


bool
Block::operator==(const Block& other) const
{
	return _location == other._location && _size == other._size;
}

/**
 * Return the bounding box representing this Block in XY
 */
util::rect<int> Block::getBoundingBox()
{
	return util::rect<int>(_location.x, _location.y,
		_location.x + _size.x - 1, _location.y + _size.y - 1);
}



std::size_t hash_value(const Block& block)
{
	std::size_t seed = 0;
	boost::hash_combine(seed, util::hash_value(block.location()));
	boost::hash_combine(seed, util::hash_value(block.size()));

	return seed;
}

