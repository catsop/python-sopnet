#ifndef BLOCKS_H__
#define BLOCKS_H__

#include <pipeline/Data.h>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "Block.h"
#include "Box.h"


class Blocks : public Box<>
{
public:
	typedef std::vector<boost::shared_ptr<Block> >::iterator       iterator;

	typedef std::vector<boost::shared_ptr<Block> >::const_iterator const_iterator;
	
	Blocks();
	
	Blocks(const boost::shared_ptr<Block>& block);
	
	Blocks(const boost::shared_ptr<Blocks>& blocks);
	
	bool contains(const boost::shared_ptr<Block>& block);
	
	template<typename S>
	bool contains(const util::rect<S>& rect)
	{
		return Box<>::contains<S>(rect);
	}
	
	void add(const boost::shared_ptr<Block>& block);
	
	void addAll(const std::vector<boost::shared_ptr<Block> >& blocks);
	
	void addAll(const boost::shared_ptr<Blocks>& blocks);
	
	void remove(const boost::shared_ptr<Block>& block);
	
	const const_iterator begin() const { return _blocks.begin(); }

	iterator begin() { return _blocks.begin(); }

	const const_iterator end() const { return _blocks.end(); }

	iterator end() { return _blocks.end(); }

	unsigned int length() const { return _blocks.size(); }
	
	void dilateXY();
	
	void expand(const point3<int>& direction);

	
	boost::shared_ptr<BlockManager> getManager();
	
	std::vector<boost::shared_ptr<Block> > getBlocks();
	
	bool overlaps(const boost::shared_ptr<ConnectedComponent>& component);
	
	bool empty(){return _blocks.empty();}
	
	
private:
	bool internalAdd(const boost::shared_ptr<Block>& block);
	
	void updateBox();
	
	std::vector<boost::shared_ptr<Block> > _blocks;
	
	boost::shared_ptr<BlockManager> _blockManager;
};

#endif //BLOCKS_H__
