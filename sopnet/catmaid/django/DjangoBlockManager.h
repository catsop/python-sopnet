#ifndef DJANGO_BLOCK_MANAGER_H__
#define DJANGO_BLOCK_MANAGER_H__

#include <block/BlockManager.h>
#include <block/Box.h>
#include <block/Block.h>
#include <block/Blocks.h>
#include <block/Core.h>
#include <block/Cores.h>
#include <util/point3.hpp>
#include <boost/unordered_map.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <util/httpclient.h>

class DjangoBlockManager : public boost::enable_shared_from_this<DjangoBlockManager>,
	public BlockManager
{
public:
	/**
	 * Create and return a DjangoBlockManager given the server, stack, and project.
	 * Returns a null pointer if something goes wrong.
	 */
	static boost::shared_ptr<DjangoBlockManager>
		getBlockManager(const std::string& server,
						const unsigned int stack, const unsigned int project);

	/**
	 * This constructor is not meant to be called directly. Use getBlockManager instead.
	 */
	DjangoBlockManager(const util::point3<unsigned int> stackSize,
				const util::point3<unsigned int> blockSize,
			const util::point3<unsigned int> coreSizeInBlocks,
			const std::string server,
			const int stack, const int project);

	boost::shared_ptr<Block> blockAtLocation(const util::point3<unsigned int>& location);
	boost::shared_ptr<Block> blockAtCoordinates(const util::point3<unsigned int>& coordinates);
	boost::shared_ptr<Blocks> blocksInBox(const boost::shared_ptr<Box<> >& box);
	
	boost::shared_ptr<Core> coreAtLocation(const util::point3<unsigned int>& location);
	boost::shared_ptr<Core> coreAtCoordinates(const util::point3<unsigned int> coordinates);
	boost::shared_ptr<Cores> coresInBox(const boost::shared_ptr<Box<> > box);
	
	bool getSlicesFlag(boost::shared_ptr<Block> block);
	bool getSegmentsFlag(boost::shared_ptr<Block> block);
	bool getSolutionCostFlag(boost::shared_ptr<Block> block);
	bool getSolutionSetFlag(boost::shared_ptr<Core> core);
	
	void setSlicesFlag(boost::shared_ptr<Block> block, bool flag);
	void setSegmentsFlag(boost::shared_ptr<Block> block, bool flag);
	void setSolutionCostFlag(boost::shared_ptr<Block> block, bool flag);
	void setSolutionSetFlag(boost::shared_ptr<Core> core, bool flag);
	
private:
	
	static void appendProjectAndStack(std::ostringstream& os, const DjangoBlockManager& manager);
	static void appendProjectAndStack(std::ostringstream& os, const std::string& server,
									  const unsigned int project, const unsigned int stack);
	
	boost::shared_ptr<Block> parseBlock(const ptree& pt);
	
	boost::shared_ptr<Core> parseCore(const ptree& pt);
	
	bool getFlag(const unsigned int id, const std::string& flagName,
				 const std::string& idVar);
	void setFlag(const unsigned int id, const std::string& flagName, bool flag,
				 const std::string& idVar);
	
	const std::string _server;
	const int _stack, _project;
	
	BlockManager::PointBlockMap _blockMap;
	BlockManager::PointCoreMap _coreMap;
};

#endif //DJANGO_BLOCK_MANAGER_H__