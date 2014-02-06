#ifndef SOPNET_PYTHON_PROJECT_CONFIGURATION_H__
#define SOPNET_PYTHON_PROJECT_CONFIGURATION_H__

#include <string>
#include <util/point3.hpp>

namespace python {

/**
 * Project specific configuration to be passed to all the stateless python 
 * wrappers.
 */
class ProjectConfiguration {

public:

	/**
	 * Set the Django URL needed to access the Catsop database.
	 */
	void setDjangoUrl(const std::string& url);

	/**
	 * Get the Django URL needed to access the Catsop database.
	 */
	const std::string& getDjangoUrl() const;

	/**
	 * Set the size of a block in voxels.
	 */
	void setBlockSize(const util::point3<unsigned int>& blockSize);

	/**
	 * Get the size of a block in voxels.
	 */
	const util::point3<unsigned int>& getBlockSize() const;

private:

	std::string _djangoUrl;

	util::point3<unsigned int> _blockSize;
};

} // namespace python

#endif // SOPNET_PYTHON_PROJECT_CONFIGURATION_H__

