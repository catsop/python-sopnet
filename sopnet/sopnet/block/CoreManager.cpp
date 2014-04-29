#include "CoreManager.h"
#include <util/Logger.h>

logger::LogChannel coremanagerlog("coremanagerlog", "[CoreManager] ");

CoreManager::CoreManager(const util::point3<unsigned int>& stackSize,
						 const util::point3<unsigned int>& blockSize,
						 const util::point3<unsigned int>& coreSizeInBlocks) :
						 _stackSize(stackSize),
						 _blockSize(blockSize),
						 _coreSizeInBlocks(coreSizeInBlocks)
{
	_coreSize = util::point3<unsigned int>(coreSizeInBlocks.x * blockSize.x,
										   coreSizeInBlocks.y * blockSize.y,
										   coreSizeInBlocks.z * blockSize.z);

	_maxBlockCoordinates =
		(stackSize + blockSize - point3<unsigned int>(1u, 1u, 1u)) / blockSize;
		
	_maxCoreCoordinates = (_maxBlockCoordinates +
		coreSizeInBlocks - point3<unsigned int>(1u, 1u, 1u)) / coreSizeInBlocks;
		
	LOG_DEBUG(coremanagerlog) << "Block limit: " << stackSize << " / " << blockSize << " => " <<
		_maxBlockCoordinates << std::endl;
		
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
	if (isValidLocation(location))
	{
		point3<unsigned int> coreCoordinates = location / _coreSize;
		return coreAtCoordinates(coreCoordinates);
	}
	else
	{
		return boost::shared_ptr<Core>();
	}
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
		boost::shared_ptr<Blocks> blocks = blocksInBox(coreBox);
		
		core = boost::make_shared<Core>(_lastId++, blocks, shared_from_this());
		
		_coreMap[coordinates] = core;
		
		return core;
	}
}


boost::shared_ptr<Cores>
CoreManager::coresInBox(const boost::shared_ptr<Box<> > box)
{
	LOG_DEBUG(coremanagerlog) << "coresInBox called on " << *box << std::endl;
	
	LOG_DEBUG(coremanagerlog) << "block size: " << blockSize() << std::endl;
	LOG_DEBUG(coremanagerlog) << "core size: " << coreSize() << std::endl;
	
	
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

bool
CoreManager::isValidCoreCoordinates(const util::point3<unsigned int>& coords) const
{
	return coords < _maxCoreCoordinates;
}

/**
 * Creates an instance of a basic implementation of CoreManager, which exists only locally.
 */
CoreManager::CoreManager(const point3<unsigned int>& stackSize,
							const point3<unsigned int>& blockSize ) :
							_stackSize(stackSize), _blockSize(blockSize)
{
    _maxBlockCoordinates =
		(stackSize + blockSize - point3<unsigned int>(1u, 1u, 1u)) / blockSize;
	LOG_DEBUG(coremanagerlog) << "Limit: " << stackSize << " / " << blockSize << " => " <<
		_maxBlockCoordinates << std::endl;
}

boost::shared_ptr<Block>
CoreManager::blockAtLocation(unsigned int x, unsigned int y, unsigned int z)
{
	return blockAtLocation(util::point3<unsigned int>(x, y, z));
}

boost::shared_ptr<Block>
CoreManager::blockAtLocation(const point3<unsigned int>& location)
{
	if (isValidLocation(location))
	{
		point3<unsigned int> blockCoordinates = location / _blockSize;
		LOG_DEBUG(coremanagerlog) << "Converted location " << location << " to coordinates " <<
			blockCoordinates << std::endl;
		return blockAtCoordinates(blockCoordinates);
	}
	else
	{
		return boost::shared_ptr<Block>();
	}
}

boost::shared_ptr<Block>
CoreManager::blockAtOffset(const Block& block, const point3<int>& offset)
{
	point3<int> signedBlockCoordinates = offset + (block.location() / _blockSize);
	point3<unsigned int> blockCoordinates = signedBlockCoordinates;
	
	if (signedBlockCoordinates >= point3<int>(0,0,0) && blockCoordinates < _maxBlockCoordinates)
	{
		return blockAtCoordinates(blockCoordinates);
	}
	else
	{
		LOG_ALL(coremanagerlog) << "Invalid block coordinates: " << offset << std::endl;
		LOG_ALL(coremanagerlog) << "Max block coordinates: " << _maxBlockCoordinates << std::endl;
		return boost::shared_ptr<Block>();
	}
}

const util::point3<unsigned int>&
CoreManager::blockSize() const
{
	return _blockSize;
}

const util::point3<unsigned int>&
CoreManager::stackSize() const
{
	return _stackSize;
}

boost::shared_ptr<Blocks>
CoreManager::blocksInBox(const boost::shared_ptr< Box<unsigned int> >& box)
{
	util::point3<unsigned int> corner = box->location();
	util::point3<unsigned int> size = box->size();
	boost::shared_ptr<Blocks> blocks = boost::make_shared<Blocks>();
	for (unsigned int z = corner.z; z - corner.z < size.z; z += blockSize().z)
	{
		for (unsigned int y = corner.y; y - corner.y < size.y; y += blockSize().y)
		{
			for (unsigned int x = corner.x; x - corner.x < size.x; x += blockSize().x)
			{
				util::point3<unsigned int> coords = point3<unsigned int>(x, y, z) / blockSize();
				boost::shared_ptr<Block> block = blockAtCoordinates(coords);
				if (block)
				{
					blocks->add(block);
				}
			}
		}
	}

	return blocks;
}

const util::point3<unsigned int>&
CoreManager::maximumBlockCoordinates()
{
	return _maxBlockCoordinates;
}

bool
CoreManager::isValidCoordinates(const util::point3< unsigned int >& coords) const
{
	return coords < _maxBlockCoordinates;
}

bool CoreManager::isValidLocation(const util::point3< unsigned int >& loc) const
{
	return loc < _stackSize;
}
