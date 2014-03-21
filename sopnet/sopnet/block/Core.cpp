#include "Core.h"

Core::Core(unsigned int id) : BlocksImpl(), _id(id)
{}

Core::Core(unsigned int id, const boost::shared_ptr<BlocksImpl> blocks) :
	BlocksImpl(blocks), _id(id)
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
