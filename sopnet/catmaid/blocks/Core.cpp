#if 0
#include "Core.h"


Core::Core(unsigned int id, const boost::shared_ptr<BlocksImpl<Block> > blocks) :
	BlocksImpl<Block>(blocks), _id(id)
{}

unsigned int
Core::getId() const
{
	return _id;
}

bool Core::operator==(const Core& other) const
{
	return other.location() == location() && other.size() == size();
}

bool Core::getSolutionSetFlag()
{
	return _blockManager->getSolutionSetFlag(shared_from_this());
}

void Core::setSolutionSetFlag(const bool& flag)
{
	_blockManager->setSolutionSetFlag(shared_from_this(), flag);
}

util::point3<unsigned int>
Core::getCoordinates()
{
	util::point3<unsigned int> coreSize = _blocks[0]->getManager()->coreSize();
	return _location / coreSize;
}


//std::size_t hash_value(const Core& core)
//{
	//std::size_t seed = 0;
	//boost::hash_combine(seed, util::hash_value(core.location()));
	//boost::hash_combine(seed, util::hash_value(core.size()));

	//return seed;
//}
#endif
