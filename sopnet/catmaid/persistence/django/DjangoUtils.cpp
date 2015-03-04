#include "DjangoUtils.h"
#include <util/httpclient.h>
#include <sopnet/slices/Slice.h>
#include <imageprocessing/ConnectedComponent.h>
#include <util/Logger.h>

logger::LogChannel djangoutilslog("djangoutilslog", "[DjangoUtils] ");


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
	HttpClient client;
	boost::shared_ptr<ptree> pt;
	boost::shared_ptr<util::point3<unsigned int> > stackSize;
	int count;
	std::vector<unsigned int> stackSizeVector;

	
	appendProjectAndStack(url, server, project, stack);
	url << "/stack_info";
	
	pt = client.getPropertyTree(url.str());
	
	checkDjangoError(pt, url.str());
	
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

void
DjangoUtils::checkDjangoError(const boost::shared_ptr<ptree> pt, const std::string& url)
{
	if (!pt)
	{
		LOG_ERROR(djangoutilslog) << "JSON Error: null property tree" << std::endl;
		UTIL_THROW_EXCEPTION(
			JSONException,
			"While accessing url " << url << "\n" <<
			"Null property tree.");
	}
	else if (HttpClient::ptreeHasChild(pt, "info") && HttpClient::ptreeHasChild(pt, "traceback"))
	{
		LOG_ERROR(djangoutilslog) << "Django error: "  <<
			pt->get_child("info").get_value<std::string>() << std::endl;
		LOG_ERROR(djangoutilslog) << "    traceback: "
			<< pt->get_child("traceback").get_value<std::string>() << std::endl;
		
		UTIL_THROW_EXCEPTION(
			DjangoException,
			"While accessing url " << url << "\n" <<
			pt->get_child("info").get_value<std::string>() << "\n" <<
			pt->get_child("traceback").get_value<std::string>());
	}
	else if (HttpClient::ptreeHasChild(pt, "djerror"))
	{
		LOG_ERROR(djangoutilslog) << "Django error: "  <<
			pt->get_child("djerror").get_value<std::string>() << std::endl;
		UTIL_THROW_EXCEPTION(
			DjangoException,
			"While accessing url " << url << "\n" <<
			pt->get_child("djerror").get_value<std::string>());
	}
	else if (HttpClient::ptreeHasChild(pt, "error"))
	{
		LOG_ERROR(djangoutilslog) << "HTTP Error: " <<
			pt->get_child("error").get_value<std::string>() << std::endl;
		UTIL_THROW_EXCEPTION(
			HttpException,
			"While accessing url " << url << "\n" <<
			pt->get_child("error").get_value<std::string>());
	}
}

