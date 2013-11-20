#ifndef POINT_3_H__
#define POINT_3_H__

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/concept_check.hpp>
#include <boost/functional/hash.hpp>

#include <pipeline/Data.h>


template <class T = int>
class Point3 : public pipeline::Data
{
public:
	Point3() : x(0), y(0), z(0) {}
	
	Point3(T x_, T y_, T z_) : x(x_), y(y_), z(z_) {}
	
	Point3(const Point3<T>& point) : x(point.x), y(point.y), z(point.z) {}
	
	
	/**
	 * Point-wise addition.
	 */
	template <class S>
	Point3<T> operator+(const Point3<S>& point) const
	{
		return Point3<T>(x + point.x, y + point.y, z + point.z);
	}
	
	/**
	 * Point-wise subtraction.
	 */
	template <class S>
	Point3<T> operator-(const Point3<S>& point) const
	{
		return Point3<T>(x - point.x, y - point.y, z - point.z);
	}
	
	template <class S>
	Point3<T> operator/(const Point3<S>& point) const
	{
		return Point3<T>(x / point.x, y / point.y, z / point.z);
	}

	template <class S>
	Point3<T> operator*(const Point3<S>& point) const
	{
		return Point3<T>(x * point.x, y * point.y, z * point.z);
	}
	
	/**
	 * Point-wise equality.
	 */
	bool operator==(const Point3<T>& point) const
	{
		return x == point.x && y == point.y && z == point.z;
	}
	
	/**
	 * Point-wise comparison.
	 */
	template <class S>
	bool operator>(const Point3<S>& point) const
	{
		return x > point.x && y > point.y && z > point.z;
	}
    
	/**
	 * Point-wise comparison.
	 */
	template <class S>
	bool operator<(const Point3<S>& point) const
	{
		return x < point.x && y < point.y && z < point.z;
	}
	
	/**
	 * Point-wise comparison.
	 */
	template <class S>
	bool operator>=(const Point3<S>& point) const
	{
		return x >= point.x && y >= point.y && z >= point.z;
	}
    
	/**
	 * Point-wise comparison.
	 */
	template <class S>
	bool operator<=(const Point3<S>& point) const
	{
		return x <= point.x && y <= point.y && z <= point.z;
	}
	
	std::size_t operator()(const Point3<T>& point)
	{
		return hash_value(point);
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

template <class T>
std::size_t hash_value(Point3<T> const& point)
{
	std::size_t seed = 0;
	boost::hash_combine(seed, boost::hash_value(point.x));
	boost::hash_combine(seed, boost::hash_value(point.y));
	boost::hash_combine(seed, boost::hash_value(point.z));
	return seed;
}


#endif //POINT_3_H__
