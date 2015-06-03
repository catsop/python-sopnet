#ifndef HOST_VOLUMES_IO_HDF5_VOLUME_WRITER_H__
#define HOST_VOLUMES_IO_HDF5_VOLUME_WRITER_H__

#include <string>
#include <vigra/hdf5impex.hxx>
#include <imageprocessing/ExplicitVolume.h>

class Hdf5VolumeWriter {

public:

	Hdf5VolumeWriter(vigra::HDF5File& hdfFile) :
		_hdfFile(hdfFile) {}

protected:

	template <typename ValueType>
	void writeVolume(const ExplicitVolume<ValueType>& volume, std::string dataset) {

		vigra::TinyVector<int, 3> chunkSize(
				std::min(volume.width(),  (unsigned int)256),
				std::min(volume.height(), (unsigned int)256),
				std::min(volume.depth(),  (unsigned int)256));

		// 0 (none) ... 9 (most)
		int compressionLevel = 3;

		// the volume (compressed)
		_hdfFile.write(
				dataset,
				volume.data(),
				chunkSize,
				compressionLevel);

		vigra::MultiArray<1, float> p(3);

		// resolution
		p[0] = volume.getResolutionX();
		p[1] = volume.getResolutionY();
		p[2] = volume.getResolutionZ();
		_hdfFile.writeAttribute(
				dataset,
				"resolution",
				p);

		// offset
		p[0] = volume.getOffset().x();
		p[1] = volume.getOffset().y();
		p[2] = volume.getOffset().z();
		_hdfFile.writeAttribute(
				dataset,
				"offset",
				p);
	}

private:

	vigra::HDF5File& _hdfFile;
};

#endif // HOST_VOLUMES_IO_HDF5_VOLUME_WRITER_H__

