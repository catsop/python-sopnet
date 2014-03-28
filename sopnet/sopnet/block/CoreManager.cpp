#include "CoreManager.h"
#include <util/Logger.h>

logger::LogChannel coremanagerlog("coremanagerlog", "[CoreManager] ");

CoreManager::CoreManager(const boost::shared_ptr<BlockManager> blockManager,
						 const util::point3<unsigned int> coreSizeInBlocks) :
						 _blockManager(blockManager),
						 _coreSizeInBlocks(coreSizeInBlocks)
{
	const util::point3<unsigned int> maxBlockCoords = _blockManager->maximumBlockCoordinates();
	const util::point3<unsigned int> blockSize = blockManager->blockSize();
	_maxCoreCoordinates =
		(maxBlockCoords + coreSizeInBlocks - point3<unsigned int>(1u, 1u, 1u)) / coreSizeInBlocks;
		
	_coreSize = util::point3<unsigned int>(coreSizeInBlocks.x * blockSize.x,
										   coreSizeInBlocks.y * blockSize.y,
										   coreSizeInBlocks.z * blockSize.z);

		
	_lastId = 0;
}

boost::shared_ptr<Core>
CoreManager::coreAtLocation(unsigned int x, unsigned int y, unsigned int z)
{
	return coreAtLocation(util::point3<unsigned int>(x, y, z));
}

boost::shared_ptr<Core>
CoreManager::coreAtLocation(const util::point3<unsigned int> location)
{
	point3<unsigned int> coreCoordinates = location / _coreSize;
	return coreAtCoordinates(coreCoordinates);
}

boost::shared_ptr<Core>
CoreManager::coreAtCoordinates(const util::point3<unsigned int> coordinates)
{
	if (_coreMap.count(coordinates))
	{
		LOG_DEBUG(coremanagerlog) << "Found core at coordinates: " << coordinates << std::endl;
		return _coreMap[coordinates];
	}
	else
	{
		boost::shared_ptr<Core> core;
		util::point3<unsigned int> coreLocation = coordinates * _coreSize;
		boost::shared_ptr<Box<> > coreBox = boost::make_shared<Box<> >(coreLocation, _coreSize);
		boost::shared_ptr<Blocks> blocks = _blockManager->blocksInBox(coreBox);
		
		core = boost::make_shared<Core>(_lastId++, blocks, shared_from_this());
		
		_coreMap[coordinates] = core;
		
		return core;
	}
}


boost::shared_ptr<BlockManager>
CoreManager::getBlockManager()
{
	return _blockManager;
}

boost::shared_ptr<Cores>
CoreManager::coresInBox(const boost::shared_ptr<Box<> > box)
{
	LOG_DEBUG(coremanagerlog) << "coresInBox called on " << *box << std::endl;
	
	LOG_DEBUG(coremanagerlog) << "block size: " << _blockManager->blockSize() << std::endl;
	LOG_DEBUG(coremanagerlog) << "core size: " << _coreSize << std::endl;
	
	
	util::point3<unsigned int> corner = box->location();
	util::point3<unsigned int> size = box->size();
	boost::shared_ptr<Cores> cores = boost::make_shared<Cores>();
	
	LOG_DEBUG(coremanagerlog) << "Here we go " << *box << std::endl;
	
	for (unsigned int z = corner.z; z - corner.z < size.z; z += _coreSize.z)
	{
		for (unsigned int y = corner.y; y - corner.y < size.y; y += _coreSize.y)
		{
			for (unsigned int x = corner.x; x - corner.x < size.x; x += _coreSize.x)
			{
				util::point3<unsigned int> coords = point3<unsigned int>(x, y, z) / _coreSize;
				LOG_DEBUG(coremanagerlog) << "Looking for core at coordinates" << coords << std::endl;
				boost::shared_ptr<Core> core = coreAtCoordinates(coords);
				cores->add(core);
			}
		}
	}

	LOG_DEBUG(coremanagerlog) << "Returning " << cores->length() << " cores." << std::endl;
	
	LOG_DEBUG(coremanagerlog) << "Cores contains" << cores->asBlocks()->length() << " blocks." << std::endl;
	
	return cores;
}

const util::point3<unsigned int>&
CoreManager::coreSize()
{
	return _coreSize;
}

const util::point3<unsigned int>&
CoreManager::coreSizeInBlocks()
{
	return _coreSizeInBlocks;
}


