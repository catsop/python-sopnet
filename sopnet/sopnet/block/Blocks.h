#ifndef BLOCKS_H__
#define BLOCKS_H__

#include <pipeline/Data.h>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "Block.h"
#include "Box.h"

class BlocksImpl : public Box<>
{
	typedef std::vector<boost::shared_ptr<Block> >::iterator       iterator;
	typedef std::vector<boost::shared_ptr<Block> >::const_iterator const_iterator;
	
public:
	
	BlocksImpl();
	
	BlocksImpl(const boost::shared_ptr<Block> block);
	
	BlocksImpl(const boost::shared_ptr<BlocksImpl> blocksImpl);
	
	virtual const const_iterator begin() const { return _blocks.begin(); }

	virtual iterator begin() { return _blocks.begin(); }

	virtual const const_iterator end() const { return _blocks.end(); }

	virtual iterator end() { return _blocks.end(); }

	virtual unsigned int length() const { return _blocks.size(); }

	virtual bool contains(const boost::shared_ptr<Block>& block);
	
	template<typename S>
	bool contains(const util::rect<S>& rect)
	{
		return Box<>::contains<S>(rect);
	}
	
	virtual void add(const boost::shared_ptr<Block>& block);
	
	virtual void addAll(const std::vector<boost::shared_ptr<Block> >& blocks);
	
	virtual void addAll(const boost::shared_ptr<BlocksImpl>& blocks);
	
	virtual void remove(const boost::shared_ptr<Block>& block);

	virtual boost::shared_ptr<BlockManager> getManager();
	
	virtual std::vector<boost::shared_ptr<Block> > getBlocks();
	
	virtual bool overlaps(const boost::shared_ptr<ConnectedComponent>& component);
	
	virtual bool empty(){return _blocks.empty();}

	
protected:
	std::vector<boost::shared_ptr<Block> > _blocks;
	boost::shared_ptr<BlockManager> _blockManager;
	
private:
	bool internalAdd(const boost::shared_ptr<Block>& block);
	
	void updateBox();

};

class Blocks : public BlocksImpl
{
public:
	Blocks();
	
	Blocks(const boost::shared_ptr<Block> block);
	
	Blocks(const boost::shared_ptr<BlocksImpl> blocks);
	
	void dilateXY();
	
	void expand(const point3<int>& direction);

};

#endif //BLOCKS_H__
