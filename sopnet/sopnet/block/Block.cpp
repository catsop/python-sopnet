#include "Block.h"
#include <boost/concept_check.hpp>
#include <util/Logger.h>
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
			boost::shared_ptr<BlockManager> manager) : Box<unsigned int>(loc, blockSize(manager, loc)),
			_manager(manager), _id(id), _slicesExtracted(false), _segmentsExtracted(false)
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

bool
Block::setSlicesFlag(bool flag)
{
	bool outFlag = _slicesExtracted;
	_slicesExtracted = flag;
	return outFlag;
}

bool
Block::setSegmentsFlag(bool flag)
{
	bool outFlag = _segmentsExtracted;
	_segmentsExtracted = flag;
	return outFlag;
}

bool
Block::getSegmentsFlag()
{
	return _segmentsExtracted;
}

bool
Block::getSlicesFlag()
{
	return _slicesExtracted;
}


bool
Block::operator==(const Block& other) const
{
	return _location == other._location && _size == other._size;
}

bool Block::overlaps(const boost::shared_ptr< ConnectedComponent >& component)
{
	return component->getBoundingBox().intersects(getBoundingBox());
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

