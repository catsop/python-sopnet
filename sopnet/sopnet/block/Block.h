#ifndef BLOCK_H__
#define BLOCK_H__

#include <boost/shared_ptr.hpp>

#include <pipeline/Data.h>
#include <util/point3.hpp>
#include <sopnet/sopnet/block/BlockManager.h>
#include <vector>

using util::point3;

class Block;
class BlockManager;

class Block : public pipeline::Data
{
public:
	Block();
	
    Block(unsigned int id, boost::shared_ptr<point3<unsigned int> > loc,
			boost::shared_ptr<BlockManager> manager);

    boost::shared_ptr<point3<unsigned int> > location() const;
	boost::shared_ptr<point3<unsigned int> > size() const;
	boost::shared_ptr<BlockManager> getManager() const;
	
    unsigned int getId() const;
	
	template<typename T>
	bool contains(const util::point<T>& loc) const
	{
		util::point<unsigned int> location = *_location;
		util::point<unsigned int> size = *_size;
		util::point<unsigned int> point = loc - location;
		
		bool positive = point.x >= 0 && point.y >= 0;
		bool contained = point.x < size.x && point.y < size.y;
		
		return positive && contained;
	}
	
	template<typename T>
	bool contains(const point3<T>& loc) const
	{
		point3<unsigned int> point = loc - *_location;

		bool positive = point >= point3<unsigned int>();;
		bool contained = point < *_size;

		return positive && contained;
	}
	
	bool contains(int z) const;
	
	
	
	bool setSlicesFlag(bool flag);
	bool setSegmentsFlag(bool flag);
	
	/**
	 * Block equality is determined by size and location.
	 */
	bool operator==(const Block& other) const;

private:
	boost::shared_ptr<point3<unsigned int> > _location, _size;
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
