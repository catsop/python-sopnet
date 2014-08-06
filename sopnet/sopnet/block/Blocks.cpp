#include "Blocks.h"
#include <util/foreach.h>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <util/point3.hpp>
#include <util/Logger.h>

logger::LogChannel blockslog("blocks", "[Blocks] ");


Blocks::Blocks() : BlocksImpl<Block>()
{}

Blocks::Blocks(const boost::shared_ptr<Block> block) : BlocksImpl<Block>(block)
{}

Blocks::Blocks(const boost::shared_ptr<BlocksImpl> blocks) : BlocksImpl<Block>(blocks)
{}

Blocks::Blocks(const BlocksImpl<Block>& blocks) : BlocksImpl<Block>(blocks)
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
