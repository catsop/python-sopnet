#include "ProjectConfiguration.h"

namespace python {

void
ProjectConfiguration::setBackendType(BackendType type) {

	_backendType = type;
}

ProjectConfiguration::BackendType
ProjectConfiguration::getBackendType() const {

	return _backendType;
}

void
ProjectConfiguration::setCatmaidHost(const std::string& url) {

	_catmaidHost = url;
}

const std::string&
ProjectConfiguration::getCatmaidHost() const {

	return _catmaidHost;
}

void
ProjectConfiguration::setCatmaidRawStackId(unsigned int stackId) {

	_rawStackId = stackId;
}

unsigned int
ProjectConfiguration::getCatmaidRawStackId() const {

	return _rawStackId;
}

void
ProjectConfiguration::setCatmaidMembraneStackId(unsigned int stackId) {

	_membraneStackId = stackId;
}

unsigned int
ProjectConfiguration::getCatmaidMembraneStackId() const {

	return _membraneStackId;
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


} // namespace python
