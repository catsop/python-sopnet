#ifndef CORES_H__
#define CORES_H__

#include <sopnet/block/Core.h>
#include <sopnet/block/Blocks.h>
#include <pipeline/Data.h>
#include <boost/shared_ptr.hpp>

class Cores : public BlocksImpl<Core>
{
public:
	Cores();
	
	Cores(const boost::shared_ptr<Cores> cores);
	
	boost::shared_ptr<Blocks> asBlocks();
};

#endif //CORES_H__