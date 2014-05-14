#ifndef DJANGO_UTILS_H__
#define DJANGO_UTILS_H__

#include <iostream>
#include <sstream>
#include <string>

class DjangoUtils
{
public:
	static void appendProjectAndStack(std::ostringstream& os, const std::string& server,
									  const unsigned int project, const unsigned int stack);
};

#endif //DJANGO_UTILS_H__