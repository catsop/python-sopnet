#include "DjangoUtils.h"

void
DjangoUtils::appendProjectAndStack(std::ostringstream& os, const std::string& server,
								   const unsigned int project, const unsigned int stack)
{
	os << "http://" << server << "/sopnet/" << project << "/stack/" << stack;
}
