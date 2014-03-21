#ifndef CORE_H__
#define CORE_H__
#include <sopnet/block/Blocks.h>
#include <boost/enable_shared_from_this.hpp>

class Core : public BlocksImpl, public boost::enable_shared_from_this<Core>
{
public:
	Core(unsigned int id);
	Core(unsigned int id, const boost::shared_ptr<BlocksImpl> blocks);

	boost::shared_ptr<Blocks> dilateXYBlocks();
	
	unsigned int getId();
	
private:
	const unsigned int _id;
};

#endif //CORE_H__