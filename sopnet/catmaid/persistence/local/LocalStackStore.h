#ifndef SOPNET_CATMAIDSOPNET_PERSISTENCE_LOCAL_STACK_STORE_H__
#define SOPNET_CATMAIDSOPNET_PERSISTENCE_LOCAL_STACK_STORE_H__

#include <boost/filesystem.hpp>
#include <string>

#include <catmaid/persistence/StackStore.h>

class LocalStackStore : public StackStore
{

public:
	/**
	 * Create a StackStore that is backed by image files in the given directory.
	 */
	LocalStackStore(std::string directory);

private:
	boost::shared_ptr<Image> getImage(util::box<unsigned int, 2> bound,
									  unsigned int section);
	
	/**
	 * A vector containing the image paths, instantiated on construction.
	 */
	std::vector<boost::filesystem::path> _imagePaths;
};

#endif // SOPNET_CATMAIDSOPNET_PERSISTENCE_LOCAL_STACK_STORE_H__

