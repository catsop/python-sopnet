#include "Blocks.h"
#include <util/foreach.h>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <util/point3.hpp>
#include <util/Logger.h>

logger::LogChannel blockslog("blocks", "[Blocks] ");

BlocksImpl::BlocksImpl() : _blockManager(boost::shared_ptr<BlockManager>())
{}

BlocksImpl::BlocksImpl(const boost::shared_ptr<Block> block) : _blockManager(block->getManager())
{
	add(block);
}

BlocksImpl::BlocksImpl(const boost::shared_ptr<BlocksImpl> blocksImpl) :
	_blockManager(blocksImpl->getManager())
{
	addAll(blocksImpl->_blocks);
}



bool BlocksImpl::internalAdd(const boost::shared_ptr<Block>& block)
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

void BlocksImpl::add(const boost::shared_ptr<Block>& block)
{
	if (internalAdd(block))
	{
		updateBox();
	}
}

void BlocksImpl::addAll(const std::vector<boost::shared_ptr<Block> >& blocks)
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

void BlocksImpl::addAll(const boost::shared_ptr<BlocksImpl>& blocks)
{
	addAll(blocks->_blocks);
}


void BlocksImpl::remove(const boost::shared_ptr<Block>& otherBlock)
{
	boost::shared_ptr<Block> eraseBlock;
	foreach (boost::shared_ptr<Block> block, _blocks)
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

bool
BlocksImpl::contains(const boost::shared_ptr<Block>& otherBlock)
{
	foreach(boost::shared_ptr<Block> block, _blocks)
	{
		if (*block == *otherBlock)
		{
			return true;
		}
	}
	
	return false;
}

void BlocksImpl::updateBox()
{
	if (_blocks.empty())
	{
		_location = util::point3<unsigned int>(0,0,0);
		_size = util::point3<unsigned int>(0,0,0);
	}
	else
	{
		util::point3<unsigned int> minPoint(_blocks[0]->location()), maxPoint(_blocks[0]->location());
		
		foreach (boost::shared_ptr<Block> block, _blocks)
		{
			minPoint = minPoint.min(block->location());
			maxPoint = maxPoint.max(block->location() + block->size());
		}
		
		_location = minPoint;
		_size = maxPoint - minPoint;
	}
}

bool
BlocksImpl::overlaps(const boost::shared_ptr< ConnectedComponent >& component)
{
	foreach (boost::shared_ptr<Block> block, _blocks)
	{
		if (block->overlaps(component))
		{
			return true;
		}
	}
	
	return false;
}

std::vector<boost::shared_ptr<Block> >
BlocksImpl::getBlocks()
{
	return _blocks;
}


boost::shared_ptr<BlockManager>
BlocksImpl::getManager()
{
	return _blockManager;
}

Blocks::Blocks() : BlocksImpl()
{}

Blocks::Blocks(const boost::shared_ptr<Block> block) : BlocksImpl(block)
{}

Blocks::Blocks(const boost::shared_ptr<BlocksImpl> blocks) : BlocksImpl(blocks)
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
