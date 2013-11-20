#ifndef LOCAL_BLOCK_MANAGER_H__
#define LOCAL_BLOCK_MANAGER_H__

#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include "BlockManager.h"
#include <sopnet/block/Block.h>
#include <sopnet/block/Point3.h>

typedef boost::unordered_map<Point3<int>, boost::shared_ptr<Block> > PointBlockMap;

/**
 * A Local-only BlockManager. This class is intended to be used for testing the Catmaid-Sopnet
 * interoperability code.
 */

class LocalBlockManager : public BlockManager, public boost::enable_shared_from_this<LocalBlockManager>
{
public:
	LocalBlockManager(boost::shared_ptr<Point3<int> > stackSize,
					  boost::shared_ptr<Point3<int> > blockSize);
	
	boost::shared_ptr<Block> blockAtCoordinates(const boost::shared_ptr<Point3<int> >& coordinates);

private:
	boost::shared_ptr<PointBlockMap> _blockMap;
	unsigned int _lastId;
};

#endif //LOCAL_BLOCK_MANAGER_H__