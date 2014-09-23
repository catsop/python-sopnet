#ifndef LOCAL_BLOCK_MANAGER_H__
#define LOCAL_BLOCK_MANAGER_H__

#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "BlockManager.h"
#include <sopnet/block/Block.h>
#include <sopnet/block/Core.h>
#include <util/point3.hpp>

using util::point3;

/**
 * A Local-only BlockManager. This class is intended to be used for testing the Catmaid-Sopnet
 * interoperability code.
 */

class LocalBlockManager : public BlockManager, public boost::enable_shared_from_this<LocalBlockManager>
{
public:
	LocalBlockManager(const point3<unsigned int>& stackSize,
					  const point3<unsigned int>& blockSize,
					const point3<unsigned int>& coreSizeInBlocks);
	
	boost::shared_ptr<Block> blockAtCoordinates(const point3<unsigned int>& coordinates);
	
	boost::shared_ptr<Core> coreAtCoordinates(const util::point3<unsigned int> coordinates);
	
	bool getSlicesFlag(boost::shared_ptr<Block> block);
	bool getSegmentsFlag(boost::shared_ptr<Block> block);
	bool getSolutionCostFlag(boost::shared_ptr<Block> block);
	bool getSolutionSetFlag(boost::shared_ptr<Core> core);
	
	void setSlicesFlag(boost::shared_ptr<Block> block, bool flag);
	void setSegmentsFlag(boost::shared_ptr<Block> block, bool flag);
	void setSolutionCostFlag(boost::shared_ptr<Block> block, bool flag);
	void setSolutionSetFlag(boost::shared_ptr<Core> core, bool flag);

private:
	BlockManager::PointBlockMap _blockMap;
	BlockManager::PointCoreMap _coreMap;
	unsigned int _lastBlockId, _lastCoreId;
	boost::unordered_map<Block, bool> _blockSlicesFlagMap, _blockSegmentsFlagMap, _blockSolutionCostFlagMap;
	boost::unordered_map<Core, bool> _coreSolutionSetFlagMap;
	
	bool getFlag(const Block& block, boost::unordered_map<Block, bool> map);
};

#endif //LOCAL_BLOCK_MANAGER_H__
