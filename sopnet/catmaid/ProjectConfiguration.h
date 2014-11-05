#ifndef SOPNET_PYTHON_PROJECT_CONFIGURATION_H__
#define SOPNET_PYTHON_PROJECT_CONFIGURATION_H__

#include <string>
#include <util/point3.hpp>

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
	 * Set the CATMAID host name in the form "host:port". The ":port" can be 
	 * omited if it is 80 (http).
	 */
	void setCatmaidHost(const std::string& host);

	/**
	 * Get the CATMAID host name.
	 */
	const std::string& getCatmaidHost() const;

	/**
	 * Set the CATMAID stack id of the raw data to be used in the Django 
	 * backend.
	 */
	void setCatmaidRawStackId(unsigned int stackId);

	/**
	 * Get the CATMAID stack id of the raw data to be used in the Django 
	 * backend.
	 */
	unsigned int getCatmaidRawStackId() const;

	/**
	 * Set the CATMAID stack id of the membrane data to be used in the Django 
	 * backend.
	 */
	void setCatmaidMembraneStackId(unsigned int stackId);

	/**
	 * Get the CATMAID stack id of the membrane data to be used in the Django 
	 * backend.
	 */
	unsigned int getCatmaidMembraneStackId() const;

	/**
	 * Set the CATMAID project id to be used in the Django backend.
	 */
	void setCatmaidProjectId(unsigned int projectId);

	/**
	 * Get the CATMAID project id to be used in the Django backend.
	 */
	unsigned int getCatmaidProjectId() const;

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

	/**
	 * Set a local directory to store the connected components of slices.
	 */
	void setComponentDirectory(const std::string& componentDirectory);

	/**
	 * Get the local directory to store the connected components of slices.
	 */
	const std::string& getComponentDirectory() const;

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

	std::string _catmaidHost;

	unsigned int _rawStackId;
	unsigned int _membraneStackId;
	unsigned int _projectId;

	util::point3<unsigned int> _blockSize;
	util::point3<unsigned int> _volumeSize;
	util::point3<unsigned int> _coreSizeInBlocks;

	std::string _componentDirectory;

	std::string _postgreSqlHost;
	std::string _postgreSqlPort;
	std::string _postgreSqlUser;
	std::string _postgreSqlPassword;
	std::string _postgreSqlDatabase;
};

#endif // SOPNET_PYTHON_PROJECT_CONFIGURATION_H__

