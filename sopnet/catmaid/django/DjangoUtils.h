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
	static void appendProjectAndStack(std::ostringstream& os, const std::string& server,
									  const unsigned int project, const unsigned int stack);
	
	static boost::shared_ptr<util::point3<unsigned int> > getStackSize(const std::string& server,
																	const unsigned int project,
																	const unsigned int stack);
	
	static util::rect<int> segmentBound(const boost::shared_ptr<Segment> segment);
};

#endif //DJANGO_UTILS_H__