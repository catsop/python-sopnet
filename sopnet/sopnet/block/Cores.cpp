#include "Cores.h"
#include <boost/make_shared.hpp>
#include <util/foreach.h>

Cores::Cores() : BlocksImpl<Core>()
{

}

Cores::Cores(const boost::shared_ptr<Cores> cores) : BlocksImpl<Core>(cores)
{

}

boost::shared_ptr<Blocks>
Cores::asBlocks()
{
	boost::shared_ptr<Blocks> blocks = boost::make_shared<Blocks>();
	
	foreach (boost::shared_ptr<Core> core, _blocks)
	{
		blocks->addAll(core);
	}
	
	return blocks;
}

boost::shared_ptr<CoreManager>
Cores::getCoreManager()
{
	if (_blocks.empty())
	{
		return boost::shared_ptr<CoreManager>();
	}
	else
	{
		return _blocks[0]->getCoreManager();
	}
}
