#ifndef CORE_H__
#define CORE_H__
#include <sopnet/block/Blocks.h>
#include <boost/enable_shared_from_this.hpp>

class CoreManager;

class Core : public BlocksImpl<Block>, public boost::enable_shared_from_this<Core>
{
public:
	Core(unsigned int id, const boost::shared_ptr<BlocksImpl<Block> > blocks,
		boost::shared_ptr<CoreManager> coreManager);

	boost::shared_ptr<Blocks> dilateXYBlocks();
	
	unsigned int getId();
	
	bool getSolutionSetFlag();
	
	bool setSolutionSetFlag(const bool& flag);
	
	bool operator==(const Core& other) const;
	
	boost::shared_ptr<CoreManager> getCoreManager();
	
	util::point3<unsigned int> getCoordinates();
	
private:
	const unsigned int _id;
	
	const boost::shared_ptr<CoreManager> _coreManager;
	
	bool _solutionSetFlag;
};

/**
 * Core hash value determined by mixing hash values returned by
 * util::hash_value for location and size.
 */
std::size_t hash_value(Core const& core);

#endif //CORE_H__