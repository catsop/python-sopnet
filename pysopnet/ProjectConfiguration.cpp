#include "ProjectConfiguration.h"

namespace python {

void
ProjectConfiguration::setDjangoUrl(const std::string& url) {

	_djangoUrl = url;
}

const std::string&
ProjectConfiguration::getDjangoUrl() const {

	return _djangoUrl;
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

} // namespace python
