#ifndef DJANGO_UTILS_H__
#define DJANGO_UTILS_H__

#include <iostream>
#include <sstream>
#include <string>
#include <boost/shared_ptr.hpp>
#include <util/point3.hpp>
#include <util/rect.hpp>
#include <sopnet/segments/Segment.h>

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
};

#endif //DJANGO_UTILS_H__