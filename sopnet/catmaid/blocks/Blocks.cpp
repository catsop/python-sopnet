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

void Blocks::dilate(int x, int y, int z)
{
	std::vector<boost::shared_ptr<Block> > blocks;
	std::vector<util::point3<int> > offsets;

	for (int i = -x; i <= x; i++)
		for (int j = -y; j <= y; j++)
			for (int k = -z; k <= z; k++) {

				if (i == 0 && j == 0 && k == 0)
					continue;

				offsets.push_back(util::point3<int>(i, j, k));
			}
	
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
