#ifndef LOCAL_BLOCK_MANAGER_H__
#define LOCAL_BLOCK_MANAGER_H__

#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "BlockManager.h"
#include <sopnet/block/Block.h>
#include <util/point3.hpp>

using util::point3;

typedef boost::unordered_map<point3<unsigned int>, boost::shared_ptr<Block> > PointBlockMap;

/**
 * A Local-only BlockManager. This class is intended to be used for testing the Catmaid-Sopnet
 * interoperability code.
 */

class LocalBlockManager : public BlockManager, public boost::enable_shared_from_this<LocalBlockManager>
{
public:
	LocalBlockManager(const point3<unsigned int>& stackSize,
					  const point3<unsigned int>& blockSize);
	
	boost::shared_ptr<Block> blockAtCoordinates(const point3<unsigned int>& coordinates);
	
	bool isValidZ(unsigned int z);
	
	bool isUpperBound(unsigned int z);

private:
	boost::shared_ptr<PointBlockMap> _blockMap;
	unsigned int _lastId;	
};

#endif //LOCAL_BLOCK_MANAGER_H__
