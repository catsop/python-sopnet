#include "Hdf5VolumeStore.h"

void
Hdf5VolumeStore::saveIntensities(const ExplicitVolume<float>& intensities) {

	_hdfFile.root();
	_hdfFile.cd_mk("volumes");

	writeVolume(intensities, "intensities");
}

void
Hdf5VolumeStore::saveMembranes(const ExplicitVolume<float>& membranes) {

	_hdfFile.root();
	_hdfFile.cd_mk("volumes");

	writeVolume(membranes, "membranes");
}

void
Hdf5VolumeStore::saveLabels(const ExplicitVolume<int>& labels) {

	_hdfFile.root();
	_hdfFile.cd_mk("volumes");

	writeVolume(labels, "labels");
}

void
Hdf5VolumeStore::retrieveIntensities(ExplicitVolume<float>& intensities) {

	_hdfFile.cd("/volumes");
	readVolume(intensities, "intensities");
}

void
Hdf5VolumeStore::retrieveMembranes(ExplicitVolume<float>& membranes) {

	_hdfFile.cd("/volumes");
	readVolume(membranes, "membranes");
}

void
Hdf5VolumeStore::retrieveLabels(ExplicitVolume<int>& labels) {

	_hdfFile.cd("/volumes");
	readVolume(labels, "labels");
}
