#include "DjangoUtils.h"
#include <util/httpclient.h>
#include <sopnet/slices/Slice.h>
#include <imageprocessing/ConnectedComponent.h>

void
DjangoUtils::appendProjectAndStack(std::ostringstream& os, const std::string& server,
								   const unsigned int project, const unsigned int stack)
{
	os << "http://" << server << "/sopnet/" << project << "/stack/" << stack;
}

boost::shared_ptr< util::point3<unsigned int> >
DjangoUtils::getStackSize(const std::string& server, const unsigned int project, const unsigned int stack)
{
	std::ostringstream url;
	boost::shared_ptr<ptree> pt;
	boost::shared_ptr<util::point3<unsigned int> > stackSize;
	
	appendProjectAndStack(url, server, project, stack);
	url << "/stack_info";
	
	pt = HttpClient::getPropertyTree(url.str());
	
	if (HttpClient::checkDjangoError(pt))
	{
		stackSize = boost::make_shared<util::point3<unsigned int> >(0,0,0);
	}
	else
	{
		int count;
		std::vector<unsigned int> stackSizeVector;
		count = HttpClient::ptreeVector<unsigned int>(pt->get_child("stack_size"), stackSizeVector);
		if (count >= 3)
		{
			stackSize = boost::make_shared<util::point3<unsigned int> >(stackSizeVector[0],
																		stackSizeVector[1],
																		stackSizeVector[2]);
		}
		else
		{
			stackSize = boost::make_shared<util::point3<unsigned int> >(0,0,0);
		}
	}
	
	return stackSize;
}

util::rect<int>
DjangoUtils::segmentBound(const boost::shared_ptr<Segment> segment)
{
	util::rect<int> bound = segment->getSlices()[0]->getComponent()->getBoundingBox();
	foreach (boost::shared_ptr<Slice> slice, segment->getSlices())
	{
		bound.fit(slice->getComponent()->getBoundingBox());
	}
	return bound;
}
