#ifndef BOX_H__
#define BOX_H__

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include <pipeline/Data.h>
#include <util/point3.hpp>
#include <util/point.hpp>
#include <util/rect.hpp>

using boost::make_shared;
using util::point3;
using util::point;

template<class T = unsigned int>
class Box : public pipeline::Data
{
public:
	Box() : _location(make_shared<point3<T> >(0, 0, 0)), _size(make_shared<point3<T> >(0, 0, 0)) {}
	
	Box(boost::shared_ptr<point3<T> > location, boost::shared_ptr<point3<T> > size) :
		_location(location), _size(size) {}

	template<class S>
	Box(const Box<S>& box) :
		_location(make_shared<point3<T> >(box._location->x, box._location->y, box._location->z)),
		_size(make_shared<point3<T> >(box._size->x, box._size->y, box._size->z)) {}
	
	
	const point3<T> location() const
	{
		return *_location;
	}
	
	const point3<T> size() const
	{
		return *_size;
	}
	
	template<typename S>
	bool contains(const point<S>& loc) const
	{
		point<T> location = *_location;
		point<T> size = *_size;
		point<T> point = loc - location;
		
		bool positive = point.x >= 0 && point.y >= 0;
		bool contained = point.x < size.x && point.y < size.y;
		
		return positive && contained;
	}
	
	template<typename S>
	bool contains(const point3<S>& loc) const
	{
		point3<T> point = loc - *_location;

		bool positive = point >= point3<T>();
		bool contained = point < *_size;

		return positive && contained;
	}
	
	
protected:
	boost::shared_ptr<point3<T> > _location;
	boost::shared_ptr<point3<T> > _size;
};


#endif //BOX_H__