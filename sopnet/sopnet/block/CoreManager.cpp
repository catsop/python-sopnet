#include "CoreManager.h"

CoreManager::CoreManager(const boost::shared_ptr<BlockManager> blockManager,
						 const util::point3<unsigned int> coreSizeInBlocks) :
						 _blockManager(blockManager),
						 _coreSize(coreSizeInBlocks)
{
	const util::point3<unsigned int> maxBlockCoords = _blockManager->maximumBlockCoordinates();
	_maxCoreCoordinates =
		(maxBlockCoords + coreSizeInBlocks - point3<unsigned int>(1u, 1u, 1u)) / coreSizeInBlocks;
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
		return _coreMap[coordinates];
	}
	else
	{
		boost::shared_ptr<Core> core;
		util::point3<unsigned int> blockCoordinates = util::point3<unsigned int>(coordinates);
		util::point3<unsigned int> expandBy = _coreSize - util::point3<unsigned int>(1, 1, 1);
		boost::shared_ptr<Blocks> blocks = boost::make_shared<Blocks>();
		boost::shared_ptr<Block> block;
		blockCoordinates *= _coreSize;
		
		block = _blockManager->blockAtCoordinates(blockCoordinates);
		blocks->add(block);
		blocks->expand(expandBy);
		
		core = boost::make_shared<Core>(_lastId++, blocks);
		
		_coreMap[coordinates] = core;
		
		return core;
	}
}


boost::shared_ptr<BlockManager>
CoreManager::getBlockManager()
{
	return _blockManager;
}
