#ifndef CORE_H__
#define CORE_H__
#include <sopnet/block/Blocks.h>
#include <boost/enable_shared_from_this.hpp>

class Core : public BlocksImpl<Block>, public boost::enable_shared_from_this<Core>
{
public:
	Core(unsigned int id, const boost::shared_ptr<BlocksImpl<Block> > blocks);

	unsigned int getId();
	
	bool getSolutionSetFlag();
	
	void setSolutionSetFlag(const bool& flag);
	
	bool operator==(const Core& other) const;
	
	util::point3<unsigned int> getCoordinates();
	
private:
	const unsigned int _id;
};

/**
 * Core hash value determined by mixing hash values returned by
 * util::hash_value for location and size.
 */
std::size_t hash_value(Core const& core);

#endif //CORE_H__
