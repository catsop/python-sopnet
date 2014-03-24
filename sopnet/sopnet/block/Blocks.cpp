#include "Blocks.h"
#include <util/foreach.h>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <util/point3.hpp>
#include <util/Logger.h>

logger::LogChannel blockslog("blocks", "[Blocks] ");

template<class T>
BlocksImpl<T>::BlocksImpl() : _blockManager(boost::shared_ptr<BlockManager>())
{}

template<class T>
BlocksImpl<T>::BlocksImpl(const boost::shared_ptr<T> block) : _blockManager(block->getManager())
{
	add(block);
}

template<class T>
BlocksImpl<T>::BlocksImpl(const boost::shared_ptr<BlocksImpl<T> > blocksImpl) :
	_blockManager(blocksImpl->getManager())
{
	addAll(blocksImpl->_blocks);
}

template<class T>
bool
BlocksImpl<T>::internalAdd(const boost::shared_ptr<T>& block)
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

template<class T>
void
BlocksImpl<T>::add(const boost::shared_ptr<T> block)
{
	if (internalAdd(block))
	{
		updateBox();
	}
}

template<class T>
void
BlocksImpl<T>::addAll(const std::vector<boost::shared_ptr<T> >& blocks)
{
	bool needUpdate = false;
	foreach (boost::shared_ptr<Block> block, blocks)
	{
		needUpdate = internalAdd(block) || needUpdate;
	}
	
	if (needUpdate)
	{
		updateBox();
	}
}

template<class T>
void
BlocksImpl<T>::addAll(const boost::shared_ptr<BlocksImpl<T> > blocks)
{
	addAll(blocks->_blocks);
}

template<class T>
void
BlocksImpl<T>::remove(const boost::shared_ptr<T> otherBlock)
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
				_blockManager = boost::shared_ptr<BlockManager>();
			}
			return;
		}
	}
}

template<class T>
bool
BlocksImpl<T>::contains(const boost::shared_ptr<T> otherBlock)
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

template<class T>
void
BlocksImpl<T>::updateBox()
{
	if (_blocks.empty())
	{
		_location = util::point3<unsigned int>(0,0,0);
		_size = util::point3<unsigned int>(0,0,0);
	}
	else
	{
		util::point3<unsigned int> minPoint(_blocks[0]->location()), maxPoint(_blocks[0]->location());
		
		foreach (boost::shared_ptr<T> block, _blocks)
		{
			minPoint = minPoint.min(block->location());
			maxPoint = maxPoint.max(block->location() + block->size());
		}
		
		_location = minPoint;
		_size = maxPoint - minPoint;
	}
}

template<class T>
boost::shared_ptr<BlockManager>
BlocksImpl<T>::getManager()
{
	return _blockManager;
}

Blocks::Blocks() : BlocksImpl<Block>()
{}

Blocks::Blocks(const boost::shared_ptr<Block> block) : BlocksImpl<Block>(block)
{}

Blocks::Blocks(const boost::shared_ptr<BlocksImpl> blocks) : BlocksImpl<Block>(blocks)
{}


void Blocks::dilateXY()
{
	std::vector<boost::shared_ptr<Block> > blocks;
	std::vector<util::point3<int> > offsets;
	offsets.push_back(util::point3<int>(-1, -1, 0));
	offsets.push_back(util::point3<int>(0, -1, 0));
	offsets.push_back(util::point3<int>(1, -1, 0));
	offsets.push_back(util::point3<int>(1, 0, 0));
	offsets.push_back(util::point3<int>(1, 1, 0));
	offsets.push_back(util::point3<int>(0, 1, 0));
	offsets.push_back(util::point3<int>(-1, 1, 0));
	offsets.push_back(util::point3<int>(-1, 0, 0));
	
	foreach(boost::shared_ptr<Block> block, _blocks)
	{
		foreach (const util::point3<int>& offset, offsets)
		{
			blocks.push_back(_blockManager->blockAtOffset(*block, offset));
		}
	}
	
	addAll(blocks);
}

void Blocks::expand(const util::point3<int>& direction)
{
	std::vector<boost::shared_ptr<Block> > blocks;
	
	foreach(boost::shared_ptr<Block> block, _blocks)
	{
		blocks.push_back(_blockManager->blockAtOffset(*block, direction));
	}
	
	addAll(blocks);
}
