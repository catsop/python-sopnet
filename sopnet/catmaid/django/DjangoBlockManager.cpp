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
	
	boost::shared_ptr<ptree> pt = HttpClient::getPropertyTree(os.str());
	
	if (HttpClient::checkDjangoError(pt))
	{
		LOG_ERROR(djangoblockmanagerlog) << "Django error in getBlockManager" << std::endl;
		return boost::shared_ptr<DjangoBlockManager>();
	}
	
	
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
	else
	{
		return boost::shared_ptr<DjangoBlockManager>();
	}
}


boost::shared_ptr<Block>
DjangoBlockManager::parseBlock(const ptree& pt)
{
	int signedId = pt.get_child("id").get_value<int>();
	boost::shared_ptr<Block> block;

	if (signedId >= 0)
	{
		std::vector<unsigned int> vGeometry;
		unsigned int id, geometryCount;

		id = pt.get_child("id").get_value<unsigned int>();
		geometryCount = HttpClient::ptreeVector<unsigned int>(pt.get_child("box"), vGeometry);
		
		if (geometryCount < 6)
		{
			LOG_ERROR(djangoblockmanagerlog) << "JSON: block " << id << " box field had " <<
				geometryCount << " entries, need 6" << std::endl;
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

	return block;
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
		boost::shared_ptr<ptree> pt;
		boost::shared_ptr<Block> block;

		appendProjectAndStack(os, *this);
		os << "/block_at_location?x=" << loc.x << "&y=" << loc.y << "&z=" <<loc.z;

		pt = HttpClient::getPropertyTree(os.str());

		if (!HttpClient::checkDjangoError(pt))
		{
			block = parseBlock(*pt);
			_blockMap[loc] = block;
			return block;
		}
		else
		{
			LOG_ERROR(djangoblockmanagerlog) << "Django error in blockAtLocation " << location <<
				std::endl;
			return boost::shared_ptr<Block>();
		}
	}
}

boost::shared_ptr<Block>
DjangoBlockManager::blockAtCoordinates(const util::point3<unsigned int>& coordinates)
{
	return blockAtLocation(coordinates * blockSize());
}

boost::shared_ptr<Blocks>
DjangoBlockManager::blocksInBox(const boost::shared_ptr<Box<> >& box)
{
    boost::shared_ptr<Blocks> blocks = boost::make_shared<Blocks>();
	boost::shared_ptr<ptree> pt;
	std::ostringstream os;
	
	appendProjectAndStack(os, *this);
	
	os << "/blocks_in_box?xmin=" << box->location().x << "&ymin=" << box->location().y << "&zmin=" <<
		box->location().z << "&width=" << box->size().x << "&height=" << box->size().y <<
		"&depth=" << box->size().z;
	
	pt = HttpClient::getPropertyTree(os.str());
	
	if (HttpClient::checkDjangoError(pt))
	{
		LOG_ERROR(djangoblockmanagerlog) << "Django error in blocksInBox: " << *box << std::endl;
	}
	else
	{
		foreach (ptree::value_type v, pt->get_child("blocks"))
		{
			boost::shared_ptr<Block> block = parseBlock(v.second);

			if (block && !_blockMap.count(block->location()))
			{
				_blockMap[block->location()] = block;
			}
			
			blocks->add(block);
		}
	}

	return blocks;
}

boost::shared_ptr<Core>
DjangoBlockManager::parseCore(const ptree& pt)
{
	int signedId = pt.get_child("id").get_value<int>();
	boost::shared_ptr<Core> core;

	if (signedId >= 0)
	{
		
		std::vector<unsigned int> vGeometry;
		unsigned int id, geometryCount;

		id = pt.get_child("id").get_value<unsigned int>();
		geometryCount = HttpClient::ptreeVector<unsigned int>(pt.get_child("box"),
																vGeometry);
		
		if (geometryCount < 6)
		{
			LOG_ERROR(djangoblockmanagerlog) << "JSON: core " << id << " box field had " <<
				geometryCount << " entries, need 6" << std::endl;
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

	return core;
}

boost::shared_ptr<Core>
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
		boost::shared_ptr<ptree> pt;

		appendProjectAndStack(os, *this);
		os << "/core_at_location?x=" << loc.x << "&y=" << loc.y << "&z=" <<loc.z;

		pt = HttpClient::getPropertyTree(os.str());
		if (HttpClient::checkDjangoError(pt))
		{
			LOG_ERROR(djangoblockmanagerlog) << "Django error in coreAtLocation" << std::endl;
			return boost::shared_ptr<Core>();
		}
		else
		{
			boost::shared_ptr<Core> core = parseCore(*pt);
			_coreMap[loc] = core;
			return core;
		}
	}
}

boost::shared_ptr<Core>
DjangoBlockManager::coreAtCoordinates(const util::point3<unsigned int> coordinates)
{
	return coreAtLocation(coordinates * coreSize());
}

boost::shared_ptr<Cores>
DjangoBlockManager::coresInBox(const boost::shared_ptr<Box<> > box)
{
    boost::shared_ptr<Cores> cores;
	boost::shared_ptr<ptree> pt;
	std::ostringstream os;
	
	appendProjectAndStack(os, *this);
	
	os << "/cores_in_box?xmin=" << box->location().x << "&ymin=" << box->location().y << "&zmin=" <<
		box->location().z << "&width=" << box->size().x << "&height=" << box->size().y <<
		"&depth=" << box->size().z;

	pt = HttpClient::getPropertyTree(os.str());
	
	if (HttpClient::checkDjangoError(pt))
	{
		LOG_ERROR(djangoblockmanagerlog) << "Django error in coresInBox: " << *box << std::endl;
	}
	else
	{
		foreach (ptree::value_type v, pt->get_child("cores"))
		{
			boost::shared_ptr<Core> core = parseCore(v.second);

			if (core && !_coreMap.count(core->location()))
			{
				_coreMap[core->location()] = core;
			}

			cores->add(core);
		}
	}

	return cores;
}

bool DjangoBlockManager::getFlag(const unsigned int id, const std::string& flagName,
								 const std::string& idVar)
{
	std::ostringstream os;
	boost::shared_ptr<ptree> pt;
	std::string flagStatus;
	
	appendProjectAndStack(os, *this);
	os << "/get_" << flagName << "?" << idVar << "=" << id;

	pt = HttpClient::getPropertyTree(os.str());

	if (HttpClient::checkDjangoError(pt))
	{
		LOG_ERROR(djangoblockmanagerlog) << "Django error in get_"  << flagName <<
			" for " << idVar << " = " << id << std::endl;
		return false;
	}
	
	flagStatus = pt->get_child(flagName).get_value<std::string>();
	
	return flagStatus.compare("true") == 0;

}

void DjangoBlockManager::setFlag(const unsigned int id, const std::string& flagName,
								 bool flag, const std::string& idVar)
{
	std::ostringstream os;
	boost::shared_ptr<ptree> pt;
	int iFlag = flag ? 1 : 0;
	bool ok;
	
	appendProjectAndStack(os, *this);
	os << "/set_" << flagName << "?" << idVar << "=" << id << "&flag=" << iFlag;
	
	pt = HttpClient::getPropertyTree(os.str());
	
	if (HttpClient::checkDjangoError(pt))
	{
		LOG_ERROR(djangoblockmanagerlog) << "Django error in set_" << flagName <<
			" for " << idVar << " = " << id << std::endl;
		return;
	}
	
	ok = pt->get_child("ok").get_value<std::string>().compare("true") == 0;
	
	if (!ok)
	{
		LOG_ERROR(djangoblockmanagerlog) << "Got not-ok when setting " << flagName << " on " <<
			idVar << " " << id << std::endl;
	}
}


bool
DjangoBlockManager::getSegmentsFlag(boost::shared_ptr<Block> block)
{
	return getFlag(block->getId(), "segments_flag", "block_id");
}

void
DjangoBlockManager::setSegmentsFlag(boost::shared_ptr<Block> block, bool flag)
{
	setFlag(block->getId(), "segments_flag", flag, "block_id");
}

bool
DjangoBlockManager::getSlicesFlag(boost::shared_ptr<Block> block)
{
	return getFlag(block->getId(), "slices_flag", "block_id");
}

void
DjangoBlockManager::setSlicesFlag(boost::shared_ptr<Block> block, bool flag)
{
	setFlag(block->getId(), "slices_flag", flag, "block_id");
}

bool
DjangoBlockManager::getSolutionCostFlag(boost::shared_ptr<Block> block)
{
	return getFlag(block->getId(), "solution_cost_flag", "block_id");
}

void
DjangoBlockManager::setSolutionCostFlag(boost::shared_ptr<Block> block, bool flag)
{
	setFlag(block->getId(), "solution_cost_flag", flag, "block_id");
}

bool
DjangoBlockManager::getSolutionSetFlag(boost::shared_ptr<Core> core)
{
	return getFlag(core->getId(), "solution_set_flag", "core_id");
}

void
DjangoBlockManager::setSolutionSetFlag(boost::shared_ptr<Core> core, bool flag)
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

