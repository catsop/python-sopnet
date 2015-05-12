#ifndef HOST_VOLUMES_IO_HDF5_VOLUME_READER_H__
#define HOST_VOLUMES_IO_HDF5_VOLUME_READER_H__

#include <string>
#include <vigra/hdf5impex.hxx>
#include <imageprocessing/ExplicitVolume.h>

class Hdf5VolumeReader {

public:

	Hdf5VolumeReader(vigra::HDF5File& hdfFile) :
		_hdfFile(hdfFile) {}

protected:

	template <typename ValueType>
	void readVolume(ExplicitVolume<ValueType>& volume, std::string dataset, bool onlyGeometry = false) {

		// the volume
		if (!onlyGeometry)
			_hdfFile.readAndResize(dataset, volume.data());

		vigra::MultiArray<1, float> p(3);

		// resolution
		_hdfFile.readAttribute(
				dataset,
				"resolution",
				p);
		volume.setResolution(p[0], p[1], p[2]);

		// offset
		_hdfFile.readAttribute(
				dataset,
				"offset",
				p);
		volume.setOffset(p[0], p[1], p[2]);
	}

private:

	vigra::HDF5File& _hdfFile;
};

#endif // HOST_VOLUMES_IO_HDF5_VOLUME_READER_H__

