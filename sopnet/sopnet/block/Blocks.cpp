#include "Blocks.h"
#include <util/foreach.h>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <util/point3.hpp>

Blocks::Blocks() : _blockManager(boost::shared_ptr<BlockManager>())
{

}

Blocks::Blocks(const boost::shared_ptr< Block >& block) : _blockManager(block->getManager())
{
	add(block);
}

Blocks::Blocks(const boost::shared_ptr< Blocks >& blocks) :
	_blockManager(blocks->getManager())
{
	addAll(blocks->_blocks);
}


bool Blocks::internalAdd(const boost::shared_ptr< Block >& block)
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


void Blocks::add(const boost::shared_ptr<Block>& block)
{
	if (internalAdd(block))
	{
		updateBox();
	}
}

void Blocks::addAll(const std::vector<boost::shared_ptr<Block> >& blocks)
{
	bool needUpdate = false;
	foreach (boost::shared_ptr<Block> block, blocks)
	{
		needUpdate = needUpdate || internalAdd(block);
	}
	
	if (needUpdate)
	{
		updateBox();
	}
}


void Blocks::remove(const boost::shared_ptr<Block>& otherBlock)
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
Blocks::contains(const boost::shared_ptr<Block>& otherBlock)
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

void Blocks::dilateXY()
{
	std::vector<boost::shared_ptr<Block> > blocks;
	std::vector<boost::shared_ptr<util::point3<int> > > offsets;
	offsets.push_back(boost::make_shared<util::point3<int> >(-1, -1, 0));
	offsets.push_back(boost::make_shared<util::point3<int> >(0, -1, 0));
	offsets.push_back(boost::make_shared<util::point3<int> >(1, -1, 0));
	offsets.push_back(boost::make_shared<util::point3<int> >(1, 0, 0));
	offsets.push_back(boost::make_shared<util::point3<int> >(1, 1, 0));
	offsets.push_back(boost::make_shared<util::point3<int> >(0, 1, 0));
	offsets.push_back(boost::make_shared<util::point3<int> >(-1, 1, 0));
	offsets.push_back(boost::make_shared<util::point3<int> >(-1, 0, 0));
	
	foreach(boost::shared_ptr<Block> block, _blocks)
	{
		foreach (boost::shared_ptr<util::point3<int> > offset, offsets)
		{
			blocks.push_back(_blockManager->blockAtOffset(*block, offset));
		}
	}
	
	addAll(blocks);
}

void Blocks::expand(boost::shared_ptr<util::point3<int > > direction)
{
	std::vector<boost::shared_ptr<Block> > blocks;
	
	foreach(boost::shared_ptr<Block> block, _blocks)
	{
		blocks.push_back(_blockManager->blockAtOffset(*block, direction));
	}
	
	addAll(blocks);
}


void Blocks::updateBox()
{
	if (_blocks.empty())
	{
		_location = boost::make_shared<util::point3<unsigned int> >(0,0,0);
		_size = boost::make_shared<util::point3<unsigned int> >(0,0,0);
	}
	else
	{
		util::point3<unsigned int> minPoint(*_blocks[0]->location()), maxPoint(*_blocks[0]->location());
		
		foreach (boost::shared_ptr<Block> block, _blocks)
		{
			minPoint = minPoint.min(*block->location());
			maxPoint = maxPoint.max(*block->location() + *block->size());
		}
		
		_location = boost::make_shared<util::point3<unsigned int> >(minPoint);
		_size = boost::make_shared<util::point3<unsigned int> >(maxPoint - minPoint);
	}
}

bool Blocks::overlaps(const boost::shared_ptr< ConnectedComponent >& component)
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

std::vector< boost::shared_ptr< Block > > Blocks::getBlocks()
{
	return _blocks;
}


boost::shared_ptr< BlockManager > Blocks::getManager()
{
	return _blockManager;
}

