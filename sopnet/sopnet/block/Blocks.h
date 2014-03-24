#ifndef BLOCKS_H__
#define BLOCKS_H__

#include <pipeline/Data.h>
#include <vector>
#include <boost/shared_ptr.hpp>

#include "Block.h"
#include "Box.h"

template <typename T>
class BlocksImpl : public Box<>
{
	typedef std::vector<boost::shared_ptr<T> >::iterator       iterator;
	typedef std::vector<boost::shared_ptr<T> >::const_iterator const_iterator;
	
public:
	
	BlocksImpl();
	
	BlocksImpl(const boost::shared_ptr<T> block);
	
	BlocksImpl(const boost::shared_ptr<BlocksImpl<T> > blocksImpl);
	
	virtual const const_iterator begin() const { return _blocks.begin(); }

	virtual iterator begin() { return _blocks.begin(); }

	virtual const const_iterator end() const { return _blocks.end(); }

	virtual iterator end() { return _blocks.end(); }

	virtual unsigned int length() const { return _blocks.size(); }

	virtual bool contains(const boost::shared_ptr<T> block);
	
	template<typename S>
	bool contains(const util::rect<S>& rect)
	{
		return Box<>::contains<S>(rect);
	}
	
	virtual void add(const boost::shared_ptr<T> block);
	
	virtual void addAll(const std::vector<boost::shared_ptr<T> >& blocks);
	
	virtual void addAll(const boost::shared_ptr<BlocksImpl<T> > blocks);
	
	virtual void remove(const boost::shared_ptr<T> block);

	virtual boost::shared_ptr<BlockManager> getManager();
	
	virtual bool empty(){return _blocks.empty();}

	
protected:
	std::vector<boost::shared_ptr<T> > _blocks;
	boost::shared_ptr<BlockManager> _blockManager;
	
private:
	bool internalAdd(const boost::shared_ptr<T>& block);
	
	void updateBox();

};

class Blocks : public BlocksImpl<Block>
{
public:
	Blocks();
	
	Blocks(const boost::shared_ptr<Block> block);
	
	Blocks(const boost::shared_ptr<BlocksImpl> blocks);
	
	void dilateXY();
	
	void expand(const point3<int>& direction);

};

#endif //BLOCKS_H__
