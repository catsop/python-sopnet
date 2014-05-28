#include "DjangoGenerators.h"
#include <catmaid/django/DjangoUtils.h>
#include <catmaid/django/DjangoBlockManager.h>
#include <util/ProgramOptions.h>
#include <util/httpclient.h>
#include <string>
#include <iostream>


util::ProgramOption optionYesItsOKToDeleteDjangoData(
	util::_module = 			"djsopnet",
	util::_long_name = 			"itIsOKToDeleteDjangoData",
	util::_description_text =
		"Allow testing of Django-backed storage to silently delete all data in the db");

logger::LogChannel djangogeneratorslog("djangogeneratorslog", "[DjangoGenerators] ");

namespace catsoptest
{

bool clearDJSopnet(const std::string& server, const unsigned int project, const unsigned int stack)
{
	// Check if it is really, really OK with the user to do this.
	if (!optionYesItsOKToDeleteDjangoData)
	{
		bool answer = false;
		int tries = 0;
		std::cout << "For testing purposes, I would like to delete ALL data in the stack" <<
			" with id " << stack << " and the project with id " << project <<
			" on this server: " << server << std::endl;

		while (!answer && tries < 3)
		{
			std::string isItOK;
			std::cout << "Is this ok (yes/no)? ";
			std::cin >> isItOK;
			
			if (isItOK.compare("no") == 0)
			{
				return false;
			}
			else if (isItOK.compare("yes") == 0)
			{
				answer = true;
			}
			else
			{
				std::cout << "Please type either \"yes\" or \"no\"" << std::endl;
			}
		}
		
		if (!answer)
		{
			std::cout << "Too many typos, giving up" << std::endl;
			return false;
		}
	}
	
	std::ostringstream os;
	boost::shared_ptr<ptree> pt;
	
	DjangoUtils::appendProjectAndStack(os, server, project, stack);
	os << "/clear_djsopnet?sure=yes";
	pt = HttpClient::getPropertyTree(os.str());
	
	if (HttpClient::checkDjangoError(pt) ||
		pt->get_child("ok").get_value<std::string>().compare("true") != 0)
	{
		LOG_ERROR(djangogeneratorslog) << "Unable to clear Django data" << std::endl;
		return false;
	}
	else
	{
		return true;
	}
}

boost::shared_ptr<BlockManager> getNewDjangoBlockManager(const std::string& server,
												const unsigned int project,
												const unsigned int stack,
												const util::point3<unsigned int> blockSize,
												const util::point3<unsigned int> coreSizeInBlocks)
{
	if (clearDJSopnet(server, project, stack))
	{
		std::ostringstream url;
		boost::shared_ptr<ptree> pt;
		
		DjangoUtils::appendProjectAndStack(url, server, project, stack);
		url << "/setup_blocks?width=" << blockSize.x << "&height=" << blockSize.y << "&depth=" <<
			blockSize.z << "&cwidth=" << coreSizeInBlocks.x << "&cheight=" << coreSizeInBlocks.y <<
			"&cdepth=" << coreSizeInBlocks.z;
		
		pt = HttpClient::getPropertyTree(url.str());
		
		if (HttpClient::checkDjangoError(pt) ||
			pt->get_child("ok").get_value<std::string>().compare("true") != 0)
		{
			boost::shared_ptr<BlockManager> nullManager = boost::shared_ptr<BlockManager>();
			LOG_ERROR(djangogeneratorslog) << "Error during block setup for project " << project <<
				" and stack " << stack << " on server " << server << std::endl;
			return nullManager;
		}
		else
		{
			return DjangoBlockManager::getBlockManager(server, stack, project);
		}
	}
	else
	{
		boost::shared_ptr<BlockManager> nullManager = boost::shared_ptr<BlockManager>();
		return nullManager;
	}
}

};