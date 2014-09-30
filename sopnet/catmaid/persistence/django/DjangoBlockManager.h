#if 0
#ifndef DJANGO_BLOCK_MANAGER_H__
#define DJANGO_BLOCK_MANAGER_H__

#include <catmaid/persistence/BlockManager.h>
#include <util/Box.h>
#include <catmaid/blocks/Block.h>
#include <catmaid/blocks/Blocks.h>
#include <catmaid/blocks/Core.h>
#include <catmaid/blocks/Cores.h>
#include <util/point3.hpp>
#include <boost/unordered_map.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <util/httpclient.h>
#include <map>

/**
 * A CATMAID/Django-backed BlockManager. Returns Blocks whose id's are the same as the
 * database primary key.
 */
class DjangoBlockManager : public boost::enable_shared_from_this<DjangoBlockManager>,
	public BlockManager
{
public:
	/**
	 * Create and return a DjangoBlockManager given the server, stack, and project.
	 * Returns a null pointer if something goes wrong.
	 * @param server - the hostname for the CATMAID server hosting the stack in question, ie
	 *                 "catmaid:8000" for the server at dns name catmaid hosting on port 8000.
	 *                 For port 80, one could use just "catmaid"
	 * @param stack - the stack id for the stack in question
	 * @param project - the project id for the project in question
	 * @return a DjangoBlockManager that operates over the geometry of the stack in question, or 
	 * else a null shared_ptr if something goes wrong.
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
	Blocks blocksInBox(const Box<>& box);

	boost::shared_ptr<Core> coreAtLocation(const util::point3<unsigned int>& location);
	boost::shared_ptr<Core> coreAtCoordinates(const util::point3<unsigned int> coordinates);
	boost::shared_ptr<Cores> coresInBox(const Box<>& box);
	
	/**
	 * Retrieve Blocks by id. Order may not be preserved. 
	 */
	boost::shared_ptr<Blocks> blocksById(std::vector<unsigned int>& ids);
	
	/**
	 * Retrieve Cores by id. Order may not be preserved.
	 */
	boost::shared_ptr<Cores> coresById(std::vector<unsigned int>& ids);
	
	bool getSlicesFlag(boost::shared_ptr<Block> block);
	bool getSegmentsFlag(boost::shared_ptr<Block> block);
	bool getSolutionCostFlag(boost::shared_ptr<Block> block);
	bool getSolutionSetFlag(boost::shared_ptr<Core> core);
	
	void setSlicesFlag(boost::shared_ptr<Block> block, bool flag);
	void setSegmentsFlag(boost::shared_ptr<Block> block, bool flag);
	void setSolutionCostFlag(boost::shared_ptr<Block> block, bool flag);
	void setSolutionSetFlag(boost::shared_ptr<Core> core, bool flag);
	
	/**
	 * @return the name of the server that this DjangoBlockManager communicates with.
	 */
	std::string getServer();
	/**
	 * @return the stack id
	 */
	int getStack();
	
	/**
	 * @return the project id
	 */
	int getProject();
	
private:
	
	void appendProjectAndStack(std::ostringstream& os);
	
	boost::shared_ptr<Block> parseBlock(const ptree& pt);
	
	boost::shared_ptr<Core> parseCore(const ptree& pt);
	
	bool getFlag(const unsigned int id, const std::string& flagName,
				 const std::string& idVar);
	void setFlag(const unsigned int id, const std::string& flagName, bool flag,
				 const std::string& idVar);
	
	void insertBlock(const boost::shared_ptr<Block> block);
	
	void insertCore(const boost::shared_ptr<Core> core);
	
	const std::string _server;
	const int _stack, _project;
	
	BlockManager::PointBlockMap _locationBlockMap;
	BlockManager::PointCoreMap _locationCoreMap;
	std::map<unsigned int, boost::shared_ptr<Block> > _idBlockMap;
	std::map<unsigned int, boost::shared_ptr<Core> > _idCoreMap;
};

#endif //DJANGO_BLOCK_MANAGER_H__
#endif
