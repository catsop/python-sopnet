#include "BlockManagerTest.h"
#include <util/Logger.h>
#include <catmaid/blocks/Core.h>
#include <catmaid/blocks/Cores.h>
#include <catmaid/blocks/Block.h>
#include <catmaid/blocks/Blocks.h>

namespace catsoptest
{

logger::LogChannel blockmanagertestlog("blockmanagertestlog", "[BlockManagerTest] ");

BlockManagerTest::BlockManagerTest(const boost::shared_ptr<BlockManagerFactory> blockManagerFactory) :
	_blockManagerFactory(blockManagerFactory)
{
	
}

bool
BlockManagerTest::run(boost::shared_ptr<BlockManagerTestParam> arg)
{
	boost::shared_ptr<BlockManager> blockManager =
		_blockManagerFactory->createBlockManager(arg->blockSize, arg->coreSizeInBlocks);
	util::point3<unsigned int> stackSize = blockManager->stackSize();
	util::point3<unsigned int> blockSize = arg->blockSize;
	
		
	boost::shared_ptr<Block> block0 =
		blockManager->blockAtLocation(util::point3<unsigned int>(0,0,0));
	boost::shared_ptr<Block> blockNull = 
		blockManager->blockAtLocation(stackSize);
	boost::shared_ptr<Block> blockSup = 
		blockManager->blockAtLocation(stackSize - util::point3<unsigned int>(1, 1, 1));
	boost::shared_ptr<Blocks> allBlocks = blockManager->blocksInBox(
		Box<>(util::point3<unsigned int>(0,0,0), stackSize));
	
	boost::shared_ptr<Core> core0 = 
		blockManager->coreAtLocation(util::point3<unsigned int>(0,0,0));
	boost::shared_ptr<Core> coreNull =
		blockManager->coreAtLocation(stackSize);
	boost::shared_ptr<Core> coreSup = 
		blockManager->coreAtLocation(stackSize - util::point3<unsigned int>(1, 1, 1));
	boost::shared_ptr<Cores> allCores = blockManager->coresInBox(
		Box<>(util::point3<unsigned int>(0,0,0), stackSize));
	
	util::point3<unsigned int> maxBlockCoords = blockManager->maximumBlockCoordinates();
	util::point3<unsigned int> maxCoreCoords = blockManager->maximumCoreCoordinates();
	unsigned int nB = maxBlockCoords.x * maxBlockCoords.y * maxBlockCoords.z;
	unsigned int nC = maxCoreCoords.x * maxCoreCoords.y * maxCoreCoords.z;

	bool ok = true;
	
	LOG_DEBUG(blockmanagertestlog) << "Got " << allBlocks->length() << " blocks, expected " <<
		nB << std::endl;
	
	LOG_DEBUG(blockmanagertestlog) << "For stack size " << stackSize << " and block size " <<
		blockSize << ":" << std::endl;
	LOG_DEBUG(blockmanagertestlog) << "\tBlock 0: " << *block0 << std::endl;
	LOG_DEBUG(blockmanagertestlog) << "\tBlock Sup: " << *blockSup << std::endl;
	
	if (allBlocks->length() != nB)
	{
		LOG_DEBUG(blockmanagertestlog) << "Got " << allBlocks->length() << " blocks, expected " <<
			nB << std::endl;
		_reason << "Got " << allBlocks->length() << " blocks, expected " << nB <<
			std::endl;
		ok = false;
	}
	
	if (blockNull)
	{
		LOG_DEBUG(blockmanagertestlog) << "\tBlock inf (should be null): " << *blockNull <<
			std::endl;
		_reason << "Block inf (should be null): " << *blockNull << std::endl;
		//ok = false;
		return false;
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
				block->getCoordinates() << " did not match block returned by the block manager " <<
				"for those coordinates" << std::endl;
			_reason << "Block " << *block << " with coordinates " <<
				block->getCoordinates() << " did not match block returned by the block manager " <<
				"for those coordinates" << std::endl;
		}
		
		if (! (*block == *blockManager->blockAtLocation(block->location())))
		{
			ok = false;
			LOG_DEBUG(blockmanagertestlog) << "Block " << *block << " with location " <<
				block->location() << " did not match block returned by the block manager " <<
				"for that location" << std::endl;
			_reason << "Block " << *block << " with location " <<
				block->location() << " did not match block returned by the block manager " <<
				"for that location" << std::endl;
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
	return "BlockManager test";
}

std::string
BlockManagerTest::reason()
{
	std::string reason = _reason.str();
	_reason.clear();
	return reason;
}

std::vector<boost::shared_ptr<BlockManagerTestParam> >
BlockManagerTest::generateTestParameters(const util::point3<unsigned int>& stackSize)
{
	std::vector<boost::shared_ptr<BlockManagerTestParam> > testParams;
	util::point3<unsigned int> ones(1, 1, 1);
	
	// blocks and cores are the same size as the stack
	boost::shared_ptr<BlockManagerTestParam> param0 =
		boost::make_shared<BlockManagerTestParam>(stackSize, ones);
	
	// 2x2x2 blocks, single core
	boost::shared_ptr<BlockManagerTestParam> param1 = 
		boost::make_shared<BlockManagerTestParam>(stackSize / 2, ones * 2);
	
	// 2x2x2 blocks, one block per core
	boost::shared_ptr<BlockManagerTestParam> param2 = 
		boost::make_shared<BlockManagerTestParam>(stackSize / 2, ones);

	// blocks at 40% the stack size, 2x2x2 blocks per core
	boost::shared_ptr<BlockManagerTestParam> param3 = 
		boost::make_shared<BlockManagerTestParam>((stackSize * 2) / 5, ones * 2);
	
	testParams.push_back(param0);
	testParams.push_back(param1);
	testParams.push_back(param2);
	testParams.push_back(param3);
	
	return testParams;
}

};

std::ostream& operator<<(std::ostream&os, const catsoptest::BlockManagerTestParam& param)
{
	os << "block size: " << param.blockSize << ", core size in blocks: " << param.coreSizeInBlocks;
	return os;
}
