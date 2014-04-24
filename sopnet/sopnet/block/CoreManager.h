#ifndef CORE_MANAGER_H__
#define CORE_MANAGER_H__

#include <boost/shared_ptr.hpp>

#include <sopnet/block/BlockManager.h>
#include <sopnet/block/Core.h>
#include <sopnet/block/Cores.h>
#include <pipeline/all.h>
#include <util/point3.hpp>
#include <boost/unordered_map.hpp>
#include <boost/enable_shared_from_this.hpp>

class CoreManager : public pipeline::Data, public boost::enable_shared_from_this<CoreManager>
{
public:
	CoreManager(const boost::shared_ptr<BlockManager> blockManager,
				const util::point3<unsigned int> coreSizeInBlocks);
	
	boost::shared_ptr<Core> coreAtLocation(unsigned int x, unsigned int y, unsigned int z);
	boost::shared_ptr<Core> coreAtLocation(const util::point3<unsigned int> location);
	boost::shared_ptr<Core> coreAtCoordinates(const util::point3<unsigned int> coordinates);
	
	boost::shared_ptr<BlockManager> getBlockManager();
	
	boost::shared_ptr<Cores> coresInBox(const boost::shared_ptr<Box<> > box);
	
	const util::point3<unsigned int>& coreSize();
	
	const util::point3<unsigned int>& coreSizeInBlocks();
	
	bool isValidCoreCoordinates(const util::point3<unsigned int>& coords) const;
	
	bool isValidCoreLocation(const util::point3<unsigned int>& loc) const;
	
private:
	boost::unordered_map<util::point3<unsigned int>, boost::shared_ptr<Core> > _coreMap;
	boost::shared_ptr<BlockManager> _blockManager;
	util::point3<unsigned int> _coreSize, _coreSizeInBlocks, _maxCoreCoordinates;
	unsigned int _lastId;
};

#endif //CORE_MANAGER_H__