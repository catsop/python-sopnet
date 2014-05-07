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

class DjangoBlockManager : boost::enable_shared_from_this<DjangoBlockManager>, public BlockManager
{
public:
	/**
	 * Create and return a DjangoBlockManager given the server, stack, and project.
	 * Returns a null pointer if something goes wrong.
	 */
	static boost::shared_ptr<DjangoBlockManager>
		getBlockManager(const std::string& server,
						const unsigned int stack, const unsigned int project);

	boost::shared_ptr<Block> blockAtLocation(const util::point3<unsigned int>& location);
	boost::shared_ptr<Block> blockAtCoordinates(const util::point3<unsigned int>& coordinates);
	boost::shared_ptr<Blocks> blocksInBox(const boost::shared_ptr<Box<> >& box);
	
	boost::shared_ptr<Block> coreAtLocation(const util::point3<unsigned int>& location);
	boost::shared_ptr<Core> coreAtCoordinates(const util::point3<unsigned int> coordinates);
	boost::shared_ptr<Cores> coresInBox(const boost::shared_ptr<Box<> > box);
	
	bool getSlicesFlag(const boost::shared_ptr<Block> block);
	bool getSegmentsFlag(const boost::shared_ptr<Block> block);
	bool getSolutionCostFlag(const boost::shared_ptr<Block> block);
	bool getSolutionSetFlag(const boost::shared_ptr<Core> core);
	
	void setSlicesFlag(const boost::shared_ptr<Block> block, bool flag);
	void setSegmentsFlag(const boost::shared_ptr<Block> block, bool flag);
	void setSolutionCostFlag(const boost::shared_ptr<Block> block, bool flag);
	void setSolutionSetFlag(const boost::shared_ptr<Core> core, bool flag);
	
private:
	DjangoBlockManager(const util::point3<unsigned int> stackSize,
					   const util::point3<unsigned int> blockSize,
					const util::point3<unsigned int> coreSizeInBlocks,
					const std::string server,
					const int stack, const int project);
	
	static void appendProjectAndStack(std::ostringstream& os, const DjangoBlockManager& manager);
	static void appendProjectAndStack(std::ostringstream& os, const std::string& server,
									  const unsigned int project, const unsigned int stack);
	
	static void logDjangoError(boost::shared_ptr<ptree> pt, std::string id);
	static void handleNon200(const HttpClient::response& res, const std::string& url);
	
	boost::shared_ptr<Block> getBlock(const unsigned int id,
									  const util::point3<unsigned int>& loc);
	
	boost::shared_ptr<Core> getCore(const unsigned int id,
									  const util::point3<unsigned int>& loc);
	
	bool getFlag(const unsigned int id, const std::string& flagName,
				 const std::string& idVar);
	void setFlag(const unsigned int id, const std::string& flagName, bool flag,
				 const std::string& idVar);
	
	const std::string _server;
	const int _stack, _project;
	
	BlockManager::PointBlockMap _blockMap;
	BlockManager::PointBlockMap _coreMap;
};

#endif //DJANGO_BLOCK_MANAGER_H__