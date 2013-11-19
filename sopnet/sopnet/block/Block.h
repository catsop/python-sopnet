#ifndef BLOCK_H__
#define BLOCK_H__

#include <boost/shared_ptr.hpp>

#include <pipeline/Data.h>
#include <sopnet/sopnet/block/Point3.h>
#include <sopnet/sopnet/block/BlockManager.h>
#include <vector>

class Block;
class BlockManager;

class Block : public pipeline::Data
{
public:
    Block(unsigned int id, boost::shared_ptr<Point3<int> > loc,
            boost::shared_ptr<Point3<int> > size,
			boost::shared_ptr<BlockManager> manager = boost::shared_ptr<BlockManager>());

    boost::shared_ptr<Point3<int> > location();
	boost::shared_ptr<Point3<int> > size();
	
    unsigned int getId() const;

	bool contains(const boost::shared_ptr<Point3<int> >& loc);
	bool contains(int z);
	
	void setSlicesFlag(bool flag);
	void setSegmentsFlag(bool flag);
	
	bool getSlicesFlag();
	bool getSegmentsFlag();

private:
	boost::shared_ptr<Point3<int> > _location, _size;
	boost::shared_ptr<BlockManager> _manager;
    unsigned int _id;
	bool _slicesExtracted, _segmentsExtracted;
};

#endif //BLOCK_H__
