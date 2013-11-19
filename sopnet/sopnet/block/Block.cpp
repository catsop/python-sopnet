#include "Block.h"
#include <boost/concept_check.hpp>

Block::Block(unsigned int id, boost::shared_ptr<Point3<int> > loc,
			boost::shared_ptr<Point3<int> > size,
			boost::shared_ptr<BlockManager> manager) : _id(id),
			_location(loc), _size(size), _manager(manager)
{
    
}

unsigned int
Block::getId() const
{
    return _id;
}

boost::shared_ptr<Point3<int> >
Block::size()
{    
    return _size;
}

boost::shared_ptr<Point3<int> >
Block::location()
{
	return _location;
}

bool
Block::contains(const boost::shared_ptr<Point3<int> >& loc)
{
	Point3<int> point = *loc - *_location;
	
	bool positive = point >= Point3<int>();;
	bool contained = point < *_size;
	
	return positive && contained;
}

bool
Block::contains(int z)
{
	return z >= _location->z && (z - _location->z) < _size->z;
}

bool
Block::getSlicesFlag()
{
	return _slicesExtracted;
}

bool
Block::getSegmentsFlag()
{
	return _segmentsExtracted;
}

void
Block::setSlicesFlag(bool flag)
{
	_slicesExtracted = flag;
}

void
Block::setSegmentsFlag(bool flag)
{
	_segmentsExtracted = flag;
}


