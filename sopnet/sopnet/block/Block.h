#ifndef BLOCK_H__
#define BLOCK_H__

#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

#include <pipeline/Data.h>
#include <util/point3.hpp>
#include <sopnet/sopnet/block/BlockManager.h>
#include <imageprocessing/ConnectedComponent.h>
#include <vector>
#include "Box.h"

using util::point3;

class Block;
class BlockManager;

class Block : public boost::enable_shared_from_this<Block>, public Box<unsigned int>
{
public:
	Block();
	
    Block(
			unsigned int id,
			const point3<unsigned int>& loc,
			boost::shared_ptr<BlockManager> manager);

	boost::shared_ptr<BlockManager> getManager() const;
	
    unsigned int getId() const;
	
	void setSlicesFlag(bool flag);
	void setSegmentsFlag(bool flag);
	void setSolutionCostFlag(bool flag);
	
	bool getSlicesFlag();
	bool getSegmentsFlag();
	bool getSolutionCostFlag();
	
	util::point3<unsigned int> getCoordinates() const;
	
	/**
	 * Block equality is determined by size and location.
	 */
	bool operator==(const Block& other) const;
	
	bool operator<(const Block& other) const
	{
		return _id < other._id;
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
};

/**
 * Block hash value determined by mixing hash values returned by
 * util::hash_value for location and size.
 */
std::size_t hash_value(Block const& block);

#endif //BLOCK_H__
