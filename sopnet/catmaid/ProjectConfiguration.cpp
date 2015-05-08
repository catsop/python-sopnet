#include "ProjectConfiguration.h"

ProjectConfiguration::ProjectConfiguration() :

	_backendType(Django),
	_stackTypes(Membrane + 1),
	_projectId(2),
	_blockSize(256, 256, 10),
	_volumeSize(1024, 1024, 20),
	_coreSizeInBlocks(1, 1, 1),
	_componentDirectory("/tmp"),
	_postgreSqlHost(""),
	_postgreSqlPort("5432"),
	_postgreSqlUser("catsop_user"),
	_postgreSqlPassword("catsop_janelia_test"),
	_postgreSqlDatabase("catsop")
	{}

void
ProjectConfiguration::setBackendType(BackendType type) {

	_backendType = type;
}

ProjectConfiguration::BackendType
ProjectConfiguration::getBackendType() const {

	return _backendType;
}

void
ProjectConfiguration::setCatmaidStack(const StackType stackType, const StackDescription stack) {

	_stackTypes.at(stackType) = stack;
}

const StackDescription&
ProjectConfiguration::getCatmaidStack(const StackType stackType) const {

	return _stackTypes.at(stackType);
}

void
ProjectConfiguration::setCatmaidProjectId(unsigned int projectId) {

	_projectId = projectId;
}

unsigned int
ProjectConfiguration::getCatmaidProjectId() const {

	return _projectId;
}

void
ProjectConfiguration::setBlockSize(const util::point3<unsigned int>& blockSize) {

	_blockSize = blockSize;
}

const util::point3<unsigned int>&
ProjectConfiguration::getBlockSize() const {

	return _blockSize;
}

void
ProjectConfiguration::setVolumeSize(const util::point3<unsigned int>& volumeSize) {

	_volumeSize = volumeSize;
}

const util::point3<unsigned int>&
ProjectConfiguration::getVolumeSize() const {

	return _volumeSize;
}

void
ProjectConfiguration::setCoreSize(const util::point3<unsigned int>& coreSizeInBlocks)
{
	_coreSizeInBlocks = coreSizeInBlocks;
}

const util::point3<unsigned int>&
ProjectConfiguration::getCoreSize() const
{
	return _coreSizeInBlocks;
}

void
ProjectConfiguration::setComponentDirectory(const std::string& componentDirectory) {

	_componentDirectory = componentDirectory;
}

const std::string&
ProjectConfiguration::getComponentDirectory() const {

	return _componentDirectory;
}

void
ProjectConfiguration::setLocalFeatureWeights(const std::vector<double>& featureWeights) {

	_localFeatureWeights = featureWeights;
}

const std::vector<double>&
ProjectConfiguration::getLocalFeatureWeights() const {

	return _localFeatureWeights;
}

void
ProjectConfiguration::setPostgreSqlHost(const std::string& postgreSqlHost) {

	_postgreSqlHost = postgreSqlHost;
}

const std::string&
ProjectConfiguration::getPostgreSqlHost() const {

	return _postgreSqlHost;
}

void
ProjectConfiguration::setPostgreSqlPort(const std::string& postgreSqlPort) {

	_postgreSqlPort = postgreSqlPort;
}

const std::string&
ProjectConfiguration::getPostgreSqlPort() const {

	return _postgreSqlPort;
}

void
ProjectConfiguration::setPostgreSqlUser(const std::string& postgreSqlUser) {

	_postgreSqlUser = postgreSqlUser;
}

const std::string&
ProjectConfiguration::getPostgreSqlUser() const {

	return _postgreSqlUser;
}

void
ProjectConfiguration::setPostgreSqlPassword(const std::string& postgreSqlPassword) {

	_postgreSqlPassword = postgreSqlPassword;
}

const std::string&
ProjectConfiguration::getPostgreSqlPassword() const {

	return _postgreSqlPassword;
}

void
ProjectConfiguration::setPostgreSqlDatabase(const std::string& postgreSqlDatabase) {

	_postgreSqlDatabase = postgreSqlDatabase;
}

const std::string&
ProjectConfiguration::getPostgreSqlDatabase() const {

	return _postgreSqlDatabase;
}

