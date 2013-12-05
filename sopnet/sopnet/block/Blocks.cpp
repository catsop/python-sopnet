#include "Blocks.h"
#include <util/foreach.h>


Blocks::Blocks()
{

}

void Blocks::add(const boost::shared_ptr<Block>& block)
{
	if(block && !contains(block))
	{
		_blocks.push_back(block);
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
