#ifndef POINT_3_H__
#define POINT_3_H__

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/concept_check.hpp>

#include <pipeline/Data.h>

template <class T = int>
class Point3 : public pipeline::Data
{
public:
	Point3() : x(0), y(0), z(0) {}
	
	Point3(T x_, T y_, T z_) : x(x_), y(y_), z(z_) {}
	
	/**
	 * Point-wise addition.
	 */
	template <class S>
	Point3<T> operator+(Point3<S> point)
	{
		return Point3<T>(x + point.x, y + point.y, z + point.z);
	}
	
	/**
	 * Point-wise subtraction.
	 */
	template <class S>
	Point3<T> operator-(Point3<S> point)
	{
		return Point3<T>(x - point.x, y - point.y, z - point.z);
	}
	
	/**
	 * Point-wise equality.
	 */
	template <class S>
	bool operator==(Point3<S> point)
	{
		return x == point.x && y == point.y && z = point.z;
	}
	
	/**
	 * Point-wise comparison.
	 */
	template <class S>
	bool operator>(Point3<S> point)
	{
		return x > point.x && y > point.y && z > point.z;
	}
    
	/**
	 * Point-wise comparison.
	 */
	template <class S>
	bool operator<(Point3<S> point)
	{
		return x < point.x && y < point.y && z < point.z;
	}
	
	/**
	 * Point-wise comparison.
	 */
	template <class S>
	bool operator>=(Point3<S> point)
	{
		return x >= point.x && y >= point.y && z >= point.z;
	}
    
	/**
	 * Point-wise comparison.
	 */
	template <class S>
	bool operator<=(Point3<S> point)
	{
		return x <= point.x && y <= point.y && z <= point.z;
	}
	
	/**
	 * Creates a Point3 and returns a shared_ptr to it.
	 */
	static boost::shared_ptr<Point3<T> > ptrTo(T x_, T y_, T z_)
	{
		return boost::make_shared<Point3<T> >(x_, y_, z_);
	}
	
	const T x, y, z;
	
};

#endif //POINT_3_H__