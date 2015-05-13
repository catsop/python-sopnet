#ifndef SOPNET_BLOCKWISE_PROJECT_CONFIGURATION_H__
#define SOPNET_BLOCKWISE_PROJECT_CONFIGURATION_H__

#include <string>
#include <vector>
#include <util/point.hpp>
#include <blockwise/persistence/StackType.h>
#include <blockwise/persistence/StackDescription.h>

/**
 * Project specific configuration to be passed to all the stateless python 
 * wrappers.
 */
class ProjectConfiguration {

public:

	enum BackendType {

		/**
		 * Use local stores. For this, the block size, the volume size, and the 
		 * core size have to be set.
		 */
		Local,

		/**
		 * Use Django stores. For this, the URL to the Django server, the 
		 * hostname of the CATMAID instance, and the CATMAID project and stack 
		 * id have to be set.
		 */
		Django,

		/**
		 * Use PosgreSql stores. For this, the PostgreSql host name, user, 
		 * password, and database habe to be set.
		 */
		PostgreSql
	};

	/**
	 * Default constructor. Initializes all fields to sensible defaults.
	 */
	ProjectConfiguration();

	/**
	 * Set the backend type (Local or Django).
	 */
	void setBackendType(BackendType type);

	/**
	 * Get the backend type (Local or Django).
	 */
	BackendType getBackendType() const;

	/**
	 * Set the CATMAID stack description to be used.
	 */
	void setCatmaidStack(const StackType stackType, const StackDescription stack);

	/**
	 * Get the CATMAID stack description to be used.
	 */
	const StackDescription& getCatmaidStack(const StackType stackType) const;

	/**
	 * Set the size of a block in voxels.
	 */
	void setBlockSize(const util::point<unsigned int, 3>& blockSize);

	/**
	 * Get the size of a block in voxels.
	 */
	const util::point<unsigned int, 3>& getBlockSize() const;

	/**
	 * Set the size of the whole volume in voxels.
	 */
	void setVolumeSize(const util::point<unsigned int, 3>& volumeSize);

	/**
	 * Get the size of a volume in voxels.
	 */
	const util::point<unsigned int, 3>& getVolumeSize() const;

	/**
	 * Set the size of a core in blocks.
	 */
	void setCoreSize(const util::point<unsigned int, 3>& coreSizeInBlocks);

	/**
	 * Get the size of a core in blocks.
	 */
	const util::point<unsigned int, 3>& getCoreSize() const;

	/**
	 * Set a local directory to store the connected components of slices.
	 */
	void setComponentDirectory(const std::string& componentDirectory);

	/**
	 * Get the local directory to store the connected components of slices.
	 */
	const std::string& getComponentDirectory() const;

	/**
	 * Set feature weights for local stores.
	 */
	void setLocalFeatureWeights(const std::vector<double>& featureWeights);

	/**
	 * Get feature weights for local stores.
	 */
	const std::vector<double>& getLocalFeatureWeights() const;

	/**
	 * Set the PosgreSql host name. This can either be a domain name of a 
	 * directory for Unix socket communication.
	 */
	void setPostgreSqlHost(const std::string& pgHost);

	/**
	 * Get the PosgreSql host name.
	 */
	const std::string& getPostgreSqlHost() const;

	/**
	 * Set the PosgreSql port.
	 */
	void setPostgreSqlPort(const std::string& pgPort);

	/**
	 * Get the PosgreSql port.
	 */
	const std::string& getPostgreSqlPort() const;

	/**
	 * Set the PosgreSql user name.
	 */
	void setPostgreSqlUser(const std::string& pgUser);

	/**
	 * Get the PosgreSql user name.
	 */
	const std::string& getPostgreSqlUser() const;

	/**
	 * Set the PosgreSql password.
	 */
	void setPostgreSqlPassword(const std::string& pgPassword);

	/**
	 * Get the PosgreSql password.
	 */
	const std::string& getPostgreSqlPassword() const;

	/**
	 * Set the PosgreSql database name.
	 */
	void setPostgreSqlDatabase(const std::string& pgDatabase);

	/**
	 * Get the PosgreSql database name.
	 */
	const std::string& getPostgreSqlDatabase() const;

private:

	BackendType _backendType;

	std::vector<StackDescription> _stackTypes;

	util::point<unsigned int, 3> _blockSize;
	util::point<unsigned int, 3> _volumeSize;
	util::point<unsigned int, 3> _coreSizeInBlocks;

	std::string _componentDirectory;

	std::vector<double> _localFeatureWeights;

	std::string _postgreSqlHost;
	std::string _postgreSqlPort;
	std::string _postgreSqlUser;
	std::string _postgreSqlPassword;
	std::string _postgreSqlDatabase;
};

#endif // SOPNET_BLOCKWISE_PROJECT_CONFIGURATION_H__

