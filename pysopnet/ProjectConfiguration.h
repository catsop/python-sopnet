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

	/**
	 * Set the size of the whole volume in voxels.
	 */
	void setVolumeSize(const util::point3<unsigned int>& volumeSize);

	/**
	 * Get the size of a volume in voxels.
	 */
	const util::point3<unsigned int>& getVolumeSize() const;

	/**
	 * Set the size of a core in blocks.
	 */
	void setCoreSize(const util::point3<unsigned int>& coreSizeInBlocks);

	/**
	 * Get the size of a core in blocks.
	 */
	const util::point3<unsigned int>& getCoreSize() const;

private:

	std::string _djangoUrl;

	util::point3<unsigned int> _blockSize;
	util::point3<unsigned int> _volumeSize;
	util::point3<unsigned int> _coreSizeInBlocks;
};

} // namespace python

#endif // SOPNET_PYTHON_PROJECT_CONFIGURATION_H__

