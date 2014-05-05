#ifndef DJANGO_BLOCK_MANAGER_H__
#define DJANGO_BLOCK_MANAGER_H__

#include <block/BlockManager.h>
#include <block/Box.h>
#include <block/Block.h>
#include <block/Blocks.h>
#include <block/Core.h>
#include <block/Cores.h>
#include <util/point3.hpp>

class DjangoBlockManager : public BlockManager
{
public:
	/**
	 * Create and return a DjangoBlockManager given the server, stack, and project.
	 * Returns a null pointer if something goes wrong.
	 */
	static boost::shared_ptr<DjangoBlockManager>
		getBlockManager(const std::string& server,
						const unsigned int stack, const unsigned int project);

	boost::shared_ptr<Block> blockAtCoordinates(const util::point3<unsigned int>& coordinates);
	
	boost::shared_ptr<Core> coreAtCoordinates(const util::point3<unsigned int> coordinates);
	
	boost::shared_ptr<Blocks> blocksInBox(const boost::shared_ptr<Box<> >& box);
	
	boost::shared_ptr<Cores> coresInBox(const boost::shared_ptr<Box<> > box);
	
private:
	DjangoBlockManager(const util::point3<unsigned int> stackSize,
					   const util::point3<unsigned int> blockSize,
					const util::point3<unsigned int> coreSizeInBlocks,
					const std::string server,
					const int stack, const int project);
	
	const std::string _server;
	const int _stack, _project;
};

#endif //DJANGO_BLOCK_MANAGER_H__