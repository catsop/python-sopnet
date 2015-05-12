#ifndef HOST_TUBES_IO_HDF5_VOLUME_STORE_H__
#define HOST_TUBES_IO_HDF5_VOLUME_STORE_H__

#include "VolumeStore.h"
#include "Hdf5VolumeReader.h"
#include "Hdf5VolumeWriter.h"

class Hdf5VolumeStore : public VolumeStore, public Hdf5VolumeReader, public Hdf5VolumeWriter {

public:

	Hdf5VolumeStore(std::string projectFile) :
		Hdf5VolumeReader(_hdfFile),
		Hdf5VolumeWriter(_hdfFile),
		_hdfFile(
				projectFile,
				vigra::HDF5File::OpenMode::ReadWrite) {}

	void saveIntensities(const ExplicitVolume<float>& intensities) override;

	void saveLabels(const ExplicitVolume<int>& labels) override;

	void retrieveIntensities(ExplicitVolume<float>& intensities) override;

	void retrieveLabels(ExplicitVolume<int>& labels) override;

private:

	vigra::HDF5File _hdfFile;
};

#endif // HOST_TUBES_IO_HDF5_VOLUME_STORE_H__

