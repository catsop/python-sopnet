#ifndef DJANGO_UTILS_H__
#define DJANGO_UTILS_H__

#include <iostream>
#include <sstream>
#include <string>
#include <boost/shared_ptr.hpp>
#include <util/point3.hpp>
#include <util/rect.hpp>
#include <boost/property_tree/ptree.hpp>
#include <sopnet/segments/Segment.h>

#include <util/exceptions.h>

struct DjangoException : virtual Exception {};
struct HttpException : virtual Exception {};
struct JSONException : virtual Exception {};

class DjangoUtils
{
public:
	/**
	 * A helper function to append the url in the correct way to an ostringstream.
	 * @param os - the ostringstream to which the url will be appended
	 * @param server - the hostname for the server, with the port appended if it isn't 80, ie
	 *                 "catmaid:8000"
	 * @param project - the project id for the stack in question
	 * @param stack - the stack id for the stack in question
	 */
	static void appendProjectAndStack(std::ostringstream& os, const std::string& server,
									  const unsigned int project, const unsigned int stack);
	
	/**
	 * Get the stack size for the requested CATMAID stack
	 * @param server - the hostname for the server, with the port appended if it isn't 80, ie
	 *                 "catmaid:8000"
	 * @param project - the project id for the stack in question
	 * @param stack - the stack id for the stack in question
	 * @return a point3 representing the size of the stack in x,y,z
	 */
	static boost::shared_ptr<util::point3<unsigned int> > getStackSize(const std::string& server,
																	const unsigned int project,
																	const unsigned int stack);
	
	/**
	 * A helper function to return a rect that bounds the given segment in 2D
	 */
	static util::rect<int> segmentBound(const boost::shared_ptr<Segment> segment);
	
	/**
	 * Check a property tree for django errors. If an error is detected, this function
	 * will throw a DjangoException, HttpException or JSONException
	 * @param pt the property tree to check
	 */
	static void checkDjangoError(const boost::shared_ptr<boost::property_tree::ptree> pt,
		const std::string url = "");
};

#endif //DJANGO_UTILS_H__