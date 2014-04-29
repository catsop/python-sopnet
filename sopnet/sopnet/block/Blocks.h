#ifndef BLOCKS_H__
#define BLOCKS_H__

#include <pipeline/Data.h>
#include <vector>
#include <boost/shared_ptr.hpp>


#include "Block.h"
#include "Box.h"

template <class T>
class BlocksImpl : public Box<>
{
	
public:
	typedef typename std::vector<boost::shared_ptr<T> >::iterator       iterator;
	typedef typename std::vector<boost::shared_ptr<T> >::const_iterator const_iterator;

	BlocksImpl() : _blockManager(boost::shared_ptr<CoreManager>()) {}
	
	BlocksImpl(const boost::shared_ptr<T> block) : _blockManager(block->getManager())
	{
		add(block);
	}
	
	BlocksImpl(const boost::shared_ptr<BlocksImpl<T> > blocksImpl) :
		_blockManager(blocksImpl->getManager())
	{
		addAll(blocksImpl->_blocks);
	}
	
	virtual const const_iterator begin() const { return _blocks.begin(); }

	virtual iterator begin() { return _blocks.begin(); }

	virtual const const_iterator end() const { return _blocks.end(); }

	virtual iterator end() { return _blocks.end(); }

	virtual unsigned int length() const { return _blocks.size(); }

	virtual bool contains(const boost::shared_ptr<T> otherBlock)
	{
		foreach(boost::shared_ptr<T> block, _blocks)
		{
			if (*block == *otherBlock)
			{
				return true;
			}
		}
		
		return false;
	}
	
	template<typename S>
	bool contains(const util::rect<S>& rect)
	{
		return Box<>::contains<S>(rect);
	}
	
	virtual void add(const boost::shared_ptr<T> block)
	{
		if (internalAdd(block))
		{
			updateBox();
		}
	}
	
	virtual void addAll(const std::vector<boost::shared_ptr<T> >& blocks)
	{
		bool needUpdate = false;
		foreach (boost::shared_ptr<T> block, blocks)
		{
			needUpdate = internalAdd(block) || needUpdate;
		}
		
		if (needUpdate)
		{
			updateBox();
		}
	}
	
	virtual void addAll(const boost::shared_ptr<BlocksImpl<T> > blocks)
	{
		addAll(blocks->_blocks);
	}
	
	virtual void remove(const boost::shared_ptr<T> otherBlock)
	{
		boost::shared_ptr<T> eraseBlock;
		foreach (boost::shared_ptr<T> block, _blocks)
		{
			if (*block == *otherBlock)
			{
				_blocks.erase(std::remove(_blocks.begin(), _blocks.end(), block), _blocks.end());
				updateBox();
				if (_blocks.empty())
				{
					_blockManager = boost::shared_ptr<CoreManager>();
				}
				return;
			}
		}
	}

	virtual boost::shared_ptr<CoreManager> getManager()
	{
		return _blockManager;
	}
	
	virtual bool empty(){return _blocks.empty();}

	
protected:
	std::vector<boost::shared_ptr<T> > _blocks;
	boost::shared_ptr<CoreManager> _blockManager;
	
private:
	bool internalAdd(const boost::shared_ptr<T>& block)
	{
		if(block && !contains(block))
		{
			if (!_blockManager)
			{
				_blockManager = block->getManager();
			}
			_blocks.push_back(block);
			return true;
		}
		else
		{
			return false;
		}
	}
	
	void updateBox()
	{
		if (_blocks.empty())
		{
			_location = util::point3<unsigned int>(0,0,0);
			_size = util::point3<unsigned int>(0,0,0);
		}
		else
		{
			util::point3<unsigned int> minPoint(_blocks[0]->location()),
				maxPoint(_blocks[0]->location());
			
			foreach (boost::shared_ptr<T> block, _blocks)
			{
				minPoint = minPoint.min(block->location());
				maxPoint = maxPoint.max(block->location() + block->size());
			}
			
			_location = minPoint;
			_size = maxPoint - minPoint;
		}
	}

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
