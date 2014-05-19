#include "BlockManagerTest.h"
#include <util/Logger.h>
#include <block/Core.h>
#include <block/Cores.h>
#include <block/Block.h>
#include <block/Blocks.h>

namespace catsoptest
{

logger::LogChannel blockmanagertestlog("blockmanagertestlog", "[BlockManagerTest] ");

BlockManagerTest::BlockManagerTest(const boost::shared_ptr<BlockManager> blockManager) :
	_blockManager(blockManager), _reason(""),
	_stackSize(blockManager->stackSize()),
	_blockSize(blockManager->blockSize()),
	_coreSizeInBlocks(blockManager->coreSizeInBlocks())
{
	
}

bool
BlockManagerTest::test()
{
	boost::shared_ptr<BlockManager> blockManager = _blockManager;
	
	boost::shared_ptr<Block> block0 =
		blockManager->blockAtLocation(util::point3<unsigned int>(0,0,0));
	boost::shared_ptr<Block> blockNull = 
		blockManager->blockAtLocation(_stackSize);
	boost::shared_ptr<Block> blockSup = 
		blockManager->blockAtLocation(_stackSize - util::point3<unsigned int>(1, 1, 1));
	boost::shared_ptr<Blocks> allBlocks = blockManager->blocksInBox(
		boost::make_shared<Box<> >(util::point3<unsigned int>(0,0,0), _stackSize));
	
	boost::shared_ptr<Core> core0 = 
		blockManager->coreAtLocation(util::point3<unsigned int>(0,0,0));
	boost::shared_ptr<Core> coreNull =
		blockManager->coreAtLocation(_stackSize);
	boost::shared_ptr<Core> coreSup = 
		blockManager->coreAtLocation(_stackSize - util::point3<unsigned int>(1, 1, 1));
	boost::shared_ptr<Cores> allCores = blockManager->coresInBox(
		boost::make_shared<Box<> >(util::point3<unsigned int>(0,0,0), _stackSize));
	
	util::point3<unsigned int> maxBlockCoords = blockManager->maximumBlockCoordinates();
	util::point3<unsigned int> maxCoreCoords = blockManager->maximumCoreCoordinates();
	unsigned int nB = maxBlockCoords.x * maxBlockCoords.y * maxBlockCoords.z;
	unsigned int nC = maxCoreCoords.x * maxCoreCoords.y * maxCoreCoords.z;

	bool ok = true;
	
	LOG_DEBUG(blockmanagertestlog) << "Got " << allBlocks->length() << " blocks, expected " <<
		nB << std::endl;
	
	LOG_DEBUG(blockmanagertestlog) << "For stack size " << _stackSize << " and block size " <<
		_blockSize << ":" << std::endl;
	LOG_DEBUG(blockmanagertestlog) << "\tBlock 0: " << *block0 << std::endl;
	LOG_DEBUG(blockmanagertestlog) << "\tBlock Sup: " << *blockSup << std::endl;
	
	if (allBlocks->length() != nB)
	{
		LOG_DEBUG(blockmanagertestlog) << "Got " << allBlocks->length() << " blocks, expected " <<
			nB << std::endl;
		_reason << "Got " << "Got " << allBlocks->length() << " blocks, expected " << nB <<
			std::endl;
		ok = false;
	}
	
	if (blockNull)
	{
		LOG_DEBUG(blockmanagertestlog) << "\tBlock inf (should be null): " << *blockNull <<
			std::endl;
		_reason << "Block inf (should be null): " << *blockNull << std::endl;
		ok = false;
	}
	else
	{
		LOG_DEBUG(blockmanagertestlog) << "\tBlock inf is correctly null" << std::endl;
	}
	
	foreach (boost::shared_ptr<Block> block, *allBlocks)
	{
		if (! (*block == *blockManager->blockAtCoordinates(block->getCoordinates())))
		{
			ok = false;
			LOG_DEBUG(blockmanagertestlog) << "Block " << *block << " with coordinates " <<
				block->getCoordinates() << " did not match block returned by the block manager" <<
				"for those coordinates" << std::endl;
			_reason << "Block " << *block << " with coordinates " <<
				block->getCoordinates() << " did not match block returned by the block manager" <<
				"for those coordinates" << std::endl;
		}
	}
	
	foreach (boost::shared_ptr<Block> block, *allBlocks)
	{
		if (block->getSlicesFlag())
		{
			ok = false;
			LOG_DEBUG(blockmanagertestlog) << "Block " << *block << " slices flag instantiated to true"
				<< std::endl;
			_reason << "Block " << *block << " slices flag instantiated to true";
		}
		block->setSlicesFlag(true);
	}
	
	foreach (boost::shared_ptr<Block> block, *allBlocks)
	{
		if (!block->getSlicesFlag())
		{
			ok = false;
			LOG_DEBUG(blockmanagertestlog) << "Block " << *block <<
				" slices flag false, previously set true" << std::endl;
			_reason << "Block " << *block << " slices flag false, previously set true" <<
				std::endl;
		}
		if (block->getSegmentsFlag())
		{
			ok = false;
			LOG_DEBUG(blockmanagertestlog) << "Block " << *block <<
				" segments flag instantiated to true" << std::endl;
			_reason << "Block " << *block <<
				" segments flag instantiated to true" << std::endl;
		}
		block->setSegmentsFlag(true);
	}
	
	foreach (boost::shared_ptr<Block> block, *allBlocks)
	{
		if (!block->getSegmentsFlag())
		{
			ok = false;
			LOG_DEBUG(blockmanagertestlog) << "Block " << *block <<
				" segments flag false, previously set true" << std::endl;
			_reason << "Block " << *block << " segments flag false, previously set true" <<
				std::endl;
		}
		if (block->getSolutionCostFlag())
		{
			ok = false;
			LOG_DEBUG(blockmanagertestlog) << "Block " << *block <<
				" solutions flag instantiated to true" << std::endl;
			_reason << "Block " << *block << " solutions flag instantiated to true" << std::endl;
		}
		block->setSolutionCostFlag(true);
	}

	foreach (boost::shared_ptr<Block> block, *allBlocks)
	{
		if (!block->getSolutionCostFlag())
		{
			ok = false;
			LOG_DEBUG(blockmanagertestlog) << "Block " << *block <<
				" solutions flag false, previously set true" << std::endl;
			_reason <<  "Block " << *block << " solutions flag false, previously set true" <<
				std::endl;
		}
	}

	LOG_DEBUG(blockmanagertestlog) << "\tCore 0: " << *core0 << std::endl;
	LOG_DEBUG(blockmanagertestlog) << "\tCore Sup: " << *coreSup << std::endl;
	
	if (allCores->length() != nC)
	{
		LOG_DEBUG(blockmanagertestlog) << "Got " << allCores->length() << " cores, expected " <<
			nC << std::endl;
		_reason << "Got " << allCores->length() << " cores, expected " << nC << std::endl;
		ok = false;
	}
	
	if (coreNull)
	{
		LOG_DEBUG(blockmanagertestlog) << "\tCore inf (should be null): " << *coreNull << std::endl;
		_reason << "Core inf (should be null): " << *coreNull << std::endl;
		ok = false;
	}
	else
	{
		LOG_DEBUG(blockmanagertestlog) << "\tCore inf is correctly null" << std::endl;
	}
	
	foreach (boost::shared_ptr<Core> core, *allCores)
	{
		if (! (*core == *blockManager->coreAtCoordinates(core->getCoordinates())))
		{
			ok = false;
			LOG_DEBUG(blockmanagertestlog) << "Core " << *core << " with coordinates " <<
				core->getCoordinates()
				<< " did not match core returned by the block manager for those coordinates" <<
				std::endl;

			_reason << "Core " << *core << " with coordinates " << core->getCoordinates()
				<< " did not match core returned by the block manager for those coordinates" <<
				std::endl;
		}
	}
	
	foreach (boost::shared_ptr<Core> core, *allCores)
	{
		if (core->getSolutionSetFlag())
		{
			ok = false;
			LOG_DEBUG(blockmanagertestlog) << "Core " << *core <<
				" slices flag instantiated to true" << std::endl;
			_reason << "Core " << *core <<
				" slices flag instantiated to true" << std::endl;
		}
		core->setSolutionSetFlag(true);
	}
	

	foreach (boost::shared_ptr<Core> core, *allCores)
	{
		if (!core->getSolutionSetFlag())
		{
			ok = false;
			LOG_DEBUG(blockmanagertestlog) << "Core " << *core <<
				" solutions flag false, previously set true" << std::endl;
			_reason << "Core " << *core <<
				" solutions flag false, previously set true" << std::endl;
		}
	}
	
	return ok;
}

std::string
BlockManagerTest::name()
{
	std::ostringstream nameString;
	nameString << "BlockManager Test, stack size: " << _stackSize << ", block size: " << _blockSize
		<< ", and core size in blocks:" << _coreSizeInBlocks;
	return nameString.str();
}

std::string
BlockManagerTest::reason()
{
	return _reason.str();
}


};