#include "DjangoBlockManager.h"


#include <util/Logger.h>

logger::LogChannel djangoblockmanagerlog("djangoblockmanagerlog", "[DjangoBlockManager] ");

DjangoBlockManager::DjangoBlockManager(const util::point3<unsigned int> stackSize,
									   const util::point3<unsigned int> blockSize,
									   const util::point3<unsigned int> coreSizeInBlocks,
									   const std::string server, const int stack,
									   const int project):
									   BlockManager(stackSize, blockSize, coreSizeInBlocks),
									   _server(server),
									   _stack(stack),
									   _project(project)
{

}

boost::shared_ptr<DjangoBlockManager>
DjangoBlockManager::getBlockManager(const std::string& server,
									const unsigned int stack, const unsigned int project)
{
	std::vector<unsigned int> vCoreSize, vBlockSize, vStackSize;
	std::ostringstream os;
	unsigned int coreSizeCount, blockSizeCount, stackSizeCount;
	
	appendProjectAndStack(os, server, project, stack);
	
	os << "/block";
	
	HttpClient::response res = HttpClient::get(os.str());
	boost::shared_ptr<ptree> pt = HttpClient::jsonPtree(res.body);
	
	coreSizeCount = HttpClient::ptreeVector<unsigned int>(pt->get_child("core_size"), vCoreSize);
	blockSizeCount =
		HttpClient::ptreeVector<unsigned int>(pt->get_child("block_size"), vBlockSize);
	stackSizeCount =
		HttpClient::ptreeVector<unsigned int>(pt->get_child("stack_size"), vStackSize);
	
	if (coreSizeCount >= 3 && blockSizeCount >= 3 && stackSizeCount >= 3)
	{
		util::point3<unsigned int> coreSize(vCoreSize);
		util::point3<unsigned int> blockSize(vBlockSize);
		util::point3<unsigned int> stackSize(vStackSize);
		return boost::make_shared<DjangoBlockManager>(stackSize, blockSize, coreSize,
													  server, stack, project);
	}
	
}

boost::shared_ptr<Block>
DjangoBlockManager::blockAtLocation(const util::point3<unsigned int>& location)
{
	// loc is the point-wise result of location - location mod blockSize
	// in other words, it represents the infimum location in a hypothetical core containing
	// location
	util::point3<unsigned int> loc = (location / _blockSize) * _blockSize;
	
	if (_blockMap.count(loc))
	{
		return _blockMap[loc];
	}
	else
	{
		std::ostringstream os;
		int signedId;
		boost::shared_ptr<ptree> pt;
		boost::shared_ptr<Block> block;
		
		appendProjectAndStack(os, *this);
		os << "/block_at_location?x=" << loc.x << "&y=" << loc.y << "&z=" <<loc.z;
		
		HttpClient::response res = HttpClient::get(os.str());
		
		pt = HttpClient::jsonPtree(res.body);
		signedId = pt->get_child("id").get_value<int>();
		
		if (signedId >= 0)
		{
			std::vector<unsigned int> vGeometry;
			unsigned int id, geometryCount;
			bool segmentFlag, sliceFlag;

			id = pt->get_child("id").get_value<unsigned int>();
			geometryCount = HttpClient::ptreeVector<unsigned int>(pt->get_child("box"), vGeometry);
			
			if (geometryCount < 6)
			{
				block = boost::shared_ptr<Block>();
			}
			else
			{
				util::point3<unsigned int> loc = util::point3<unsigned int>(vGeometry);
				block = boost::make_shared<Block>(id, loc, shared_from_this());
			}
		}
		else
		{
			block = boost::shared_ptr<Block>();
		}
		
		_blockMap[loc] = block;
		
		return block;
	}
}

boost::shared_ptr<Block>
DjangoBlockManager::blockAtCoordinates(const util::point3<unsigned int>& coordinates)
{
	return blockAtLocation(coordinates * blockSize());
}

boost::shared_ptr<Block>
DjangoBlockManager::coreAtLocation(const util::point3<unsigned int>& location)
{
	// loc is the point-wise result of location - location mod blockSize
	// in other words, it represents the infimum location in a hypothetical core containing
	// location
	util::point3<unsigned int> loc = (location / _blockSize) * _blockSize;
	
	if (_coreMap.count(loc))
	{
		return _coreMap[loc];
	}
	else
	{
		std::ostringstream os;
		int signedId;
		boost::shared_ptr<ptree> pt;
		boost::shared_ptr<Core> core;
		
		appendProjectAndStack(os, *this);
		os << "/core_at_location?x=" << loc.x << "&y=" << loc.y << "&z=" <<loc.z;
		
		HttpClient::response res = HttpClient::get(os.str());
		
		pt = HttpClient::jsonPtree(res.body);
		signedId = pt->get_child("id").get_value<int>();
		
		if (signedId >= 0)
		{
			
			std::vector<unsigned int> vGeometry;
			unsigned int id, geometryCount;
			bool segmentFlag, sliceFlag;

			id = pt->get_child("id").get_value<unsigned int>();
			geometryCount = HttpClient::ptreeVector<unsigned int>(pt->get_child("box"), vGeometry);
			
			if (geometryCount < 6)
			{
				core = boost::shared_ptr<Core>();
			}
			else
			{
				util::point3<unsigned int> loc = util::point3<unsigned int>(vGeometry);
				util::point3<unsigned int> size = util::point3<unsigned int>(
					vGeometry[3], vGeometry[4], vGeometry[5]) - loc;
				boost::shared_ptr<Box<> > box = boost::make_shared<Box<> >(loc, size);
				boost::shared_ptr<Blocks> blocks = blocksInBox(box);
				core = boost::make_shared<Core>(id, blocks);
			}
		}
		else
		{
			core = boost::shared_ptr<Core>();
		}
		
		_coreMap[loc] = core;
		
		return core;
	}
	
	
	
}


boost::shared_ptr<Core>
DjangoBlockManager::coreAtCoordinates(const util::point3<unsigned int> coordinates)
{
	return coreAtLocation(coordinates * coreSize());
}

boost::shared_ptr<Blocks>
DjangoBlockManager::blocksInBox(const boost::shared_ptr<Box<> >& box)
{
    return BlockManager::blocksInBox(box);
}

boost::shared_ptr<Cores>
DjangoBlockManager::coresInBox(const boost::shared_ptr<Box<> > box)
{
    return BlockManager::coresInBox(box);
}

boost::shared_ptr<Block>
DjangoBlockManager::getBlock(const unsigned int id, const util::point3<unsigned int>& loc)
{
	boost::shared_ptr<Block> block;
	
	if (_blockMap.count(loc))
	{
		block = _blockMap[loc];
		if (block->getId() != id)
		{
			LOG_ERROR(djangoblockmanagerlog) << "Block at loc " << loc << " mapped to id " <<
				block->getId() << ", but " << id << " was requested" << std::endl;
		}
	}
	else
	{
		block = boost::make_shared<Block>(loc, shared_from_this());
		_blockMap[loc] = block;
	}
	return block;
}

boost::shared_ptr<Core>
DjangoBlockManager::getCore(const unsigned int id, const util::point3<unsigned int>& loc)
{
	boost::shared_ptr<Core> core;
	
	if (_coreMap.count(loc))
	{
		core = _coreMap[loc];
		if (core->getId() != id)
		{
			LOG_ERROR(djangoblockmanagerlog) << "Core at loc " << loc << " mapped to id " <<
				core->getId() << ", but " << id << " was requested" << std::endl;
		}
	}
	else
	{
		Core = boost::make_shared<Core>(loc, shared_from_this());
		_coreMap[loc] = core;
	}
	return core;
}

bool DjangoBlockManager::getFlag(const unsigned int id, const std::string& flagName,
								 const std::string& idVar)
{
	std::ostringstream os;
	boost::shared_ptr<ptree> pt;
	std::string flagStatus;
	
	appendProjectAndStack(os, *this);
	os << "/get_" << flagName << "?" << idVar << "=" << id;
	
	HttpClient::response res = HttpClient::get(os.str());
	if (res.code == 200)
	{
		pt = HttpClient::jsonPtree(res.body);
		
		if (!pt->get_child(flagName))
		{
			logDjangoError(pt, "getFlag");
		}
		
		flagStatus = pt->get_child(flagName).get_value<std::string>();
		
		return flagStatus.compare("true") == 0;
	}
	else
	{
		handleNon200(res, os.str());
		return false;
	}
}

void DjangoBlockManager::setFlag(const unsigned int id, const std::string& flagName,
								 bool flag, const std::string& idVar)
{
	std::ostringstream os;
	boost::shared_ptr<ptree> pt;
	std::string ok;
	int iFlag = flag ? 1 : 0;
	
	appendProjectAndStack(os, *this);
	os << "/set_" << flagName << "?" << idVar << "=" << id << "&flag=" << iFlag;
	HttpClient::reponse res = HttpClient::get(os.str());
	if (res.code == 200)
	{
		bool ok;
		pt = HttpClient::jsonPtree(res.body);
		
		if (!pt->get_child("ok"))
		{
			logDjangoError(pt, "setFlag");
		}
		
		ok = pt->get_child("ok").get_value<std::string>().compare("true") == 0;
		
		if (!ok)
		{
			LOG_ERROR(djangoblockmanagerlog) << "Got not-ok when setting " << flagName << " on " <<
				idVar << " " << id << std::endl;
		}
	}
	else
	{
		handleNon200(res, os.str());
	}
}


bool
DjangoBlockManager::getSegmentsFlag(const boost::shared_ptr<Block> block)
{
	return getFlag(block->getId(), "segments_flag", "block_id");
}

void
DjangoBlockManager::setSegmentsFlag(const boost::shared_ptr<Block> block, bool flag)
{
	setFlag(block->getId(), "segments_flag", flag, "block_id");
}

bool
DjangoBlockManager::getSlicesFlag(const boost::shared_ptr<Block> block)
{
	return getFlag(block->getId(), "slices_flag", "block_id");
}

void
DjangoBlockManager::setSlicesFlag(const boost::shared_ptr<Block> block, bool flag)
{
	setFlag(block->getId(), "slices_flag", flag, "block_id");
}

bool
DjangoBlockManager::getSolutionCostFlag(const boost::shared_ptr<Block> block)
{
	return getFlag(block->getId(), "solution_cost_flag", "block_id");
}

void
DjangoBlockManager::setSolutionCostFlag(const boost::shared_ptr<Block> block, bool flag)
{
	setFlag(block->getId(), "solution_cost_flag", flag, "block_id");
}

bool
DjangoBlockManager::getSolutionSetFlag(const boost::shared_ptr<Core> core)
{
	return getFlag(core->getId(), "solution_set_flag", "core_id");
}

void
DjangoBlockManager::setSolutionSetFlag(const boost::shared_ptr<Core> core, bool flag)
{
	setFlag(core->getId(), "solution_set_flag", flag, "core_id");
}



void
DjangoBlockManager::appendProjectAndStack(std::ostringstream& os,
										  const DjangoBlockManager& manager)
{
	appendProjectAndStack(os, manager._server, manager._project, manager._stack);
}

void
DjangoBlockManager::appendProjectAndStack(std::ostringstream& os, const std::string& server, 
										  const unsigned int project, const unsigned int stack)
{
	os << "http://" << server << "/sopnet/" << project << "/stack/" << stack;
}


void DjangoBlockManager::logDjangoError(boost::shared_ptr pt, std::string id)
{
	if (pt->get_child("info"))
	{
		LOG_ERROR(djangoblockmanagerlog) << "Django error (" << id << "): "  <<
			pt->get_child("info")->get_value<std::string>() << std::endl;
		LOG_ERROR(djangoblockmanagerlog) << "    traceback: "
			<< pt->get_child("traceback")->get_value<std::string>() << std::endl;
	}
	else
	{
		LOG_ERROR(djangoblockmanagerlog) << "Django error (" << id << "): " <<
			"Could not process python exception info." << std::endl;
	}
}

void
DjangoBlockManager::handleNon200(const HttpClient::response& res, const std::string& url)
{
	//TODO: throw exception?
	LOG_ERROR(djangoblockmanagerlog) << "When trying url [" << url << "], received non-OK code " <<
		res.code << std::endl;
	
}
