#ifndef BLOCK_H__
#define BLOCK_H__

#include <boost/shared_ptr.hpp>

#include <pipeline/Data.h>
#include <util/point3.hpp>
#include <sopnet/sopnet/block/BlockManager.h>
#include <imageprocessing/ConnectedComponent.h>
#include <vector>
#include "Box.h"

using util::point3;

class Block;
class BlockManager;

class Block : public Box<unsigned int>
{
public:
	Block();
	
    Block(
			unsigned int id,
			const point3<unsigned int>& loc,
			boost::shared_ptr<BlockManager> manager);

	boost::shared_ptr<BlockManager> getManager() const;
	
    unsigned int getId() const;
	
	bool overlaps(const boost::shared_ptr<ConnectedComponent>& component);
	
	bool setSlicesFlag(bool flag);
	bool setSegmentsFlag(bool flag);
	
	bool getSlicesFlag();
	
	bool getSegmentsFlag();
	
	/**
	 * Block equality is determined by size and location.
	 */
	bool operator==(const Block& other) const;
	
	bool operator<(const Block& other) const
	{
		if (_location.z < other._location.z)
		{
			return true;
		}
		
		if (_location.y < other._location.y)
		{
			return true;
		}
		
		if (_location.x < other._location.x)
		{
			return true;
		}
		
		return false;
	}
	
	/**
	* Return the bounding box representing this Block in XY
	*/
	util::rect<int> getBoundingBox();
	

private:
	static point3<unsigned int> blockSize(
		const boost::shared_ptr<BlockManager>& blockManager,
		const point3<unsigned int>& location);
	boost::shared_ptr<BlockManager> _manager;
    unsigned int _id;
	bool _slicesExtracted, _segmentsExtracted;
};

/**
 * Block hash value determined by mixing hash values returned by
 * util::hash_value for location and size.
 */
std::size_t hash_value(Block const& block);

#endif //BLOCK_H__
