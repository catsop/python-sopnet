#include "DjangoBlockManager.h"

#include <util/httpclient.h>

DjangoBlockManager::DjangoBlockManager(const util::point3<unsigned int> stackSize,
									   const util::point3<unsigned int> blockSize,
									   const util::point3<unsigned int> coreSizeInBlocks,
									   const std::string server, const int stack,
									   const int project):
									   BlockManager(stackSize, blockSize, coreSizeInBlocks),
									   _server(server),
									   _stack(stack),
									   _project(project)
{

}

boost::shared_ptr<DjangoBlockManager>
DjangoBlockManager::getBlockManager(const std::string& server,
									const unsigned int stack, const unsigned int project)
{

}

boost::shared_ptr<Block>
DjangoBlockManager::blockAtCoordinates(const util::point3<unsigned int>& coordinates)
{

}

boost::shared_ptr<Core>
DjangoBlockManager::coreAtCoordinates(const util::point3<unsigned int> coordinates)
{

}

boost::shared_ptr<Blocks>
DjangoBlockManager::blocksInBox(const boost::shared_ptr<Box<> >& box)
{
    return BlockManager::blocksInBox(box);
}

boost::shared_ptr<Cores>
DjangoBlockManager::coresInBox(const boost::shared_ptr<Box<> > box)
{
    return BlockManager::coresInBox(box);
}
