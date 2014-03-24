#include "Core.h"

Core::Core(unsigned int id, const boost::shared_ptr<BlocksImpl<Block> > blocks) :
	BlocksImpl<Block>(blocks), _id(id)
{}

boost::shared_ptr<Blocks>
Core::dilateXYBlocks()
{
	boost::shared_ptr<Blocks> blocks = boost::make_shared<Blocks>(shared_from_this());
	blocks->dilateXY();
	return blocks;
}

unsigned int
Core::getId()
{
	return _id;
}

bool Core::operator==(const Core& other) const
{
	return other.location() == location() && other.size() == size();
}
