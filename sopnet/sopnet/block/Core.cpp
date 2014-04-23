#include "Core.h"
#include "CoreManager.h"

Core::Core(unsigned int id, const boost::shared_ptr<BlocksImpl<Block> > blocks,
	const boost::shared_ptr<CoreManager> coreManager) :
	BlocksImpl<Block>(blocks), _id(id),
	_coreManager(coreManager), _solutionSetFlag(false)
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

bool Core::getSolutionSetFlag()
{
	return _solutionSetFlag;
}

bool Core::setSolutionSetFlag(const bool& flag)
{
	bool flagOut = _solutionSetFlag;
	_solutionSetFlag = flag;
	return flagOut;
}

boost::shared_ptr<CoreManager>
Core::getCoreManager()
{
	return _coreManager;
}

util::point3<unsigned int>
Core::getCoordinates()
{
	util::point3<unsigned int> coreSize = _coreManager->coreSize();
	return _location / coreSize;
}


std::size_t hash_value(const Core& core)
{
	std::size_t seed = 0;
	boost::hash_combine(seed, util::hash_value(core.location()));
	boost::hash_combine(seed, util::hash_value(core.size()));

	return seed;
}
