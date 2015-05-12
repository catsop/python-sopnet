#ifndef HOST_TUBES_IO_HDF_TUBE_STORE_H__
#define HOST_TUBES_IO_HDF_TUBE_STORE_H__

#include <vigra/hdf5impex.hxx>
#include <volumes/io/Hdf5VolumeReader.h>
#include <volumes/io/Hdf5VolumeWriter.h>
#include <tubes/io/Hdf5GraphWriter.h>
#include <tubes/io/Hdf5GraphReader.h>
#include "TubeStore.h"

class Hdf5TubeStore :
		public TubeStore,
		public Hdf5VolumeReader,
		public Hdf5VolumeWriter,
		public Hdf5GraphReader,
		public Hdf5GraphWriter {

public:

	Hdf5TubeStore(std::string projectFile) :
		Hdf5VolumeReader(_hdfFile),
		Hdf5VolumeWriter(_hdfFile),
		Hdf5GraphReader(_hdfFile),
		Hdf5GraphWriter(_hdfFile),
		_hdfFile(
				projectFile,
				vigra::HDF5File::OpenMode::ReadWrite) {}

	/**
	 * Store the given tube volumes.
	 */
	 void saveVolumes(const Volumes& volumes) override;

	/**
	 * Store the given tube features.
	 */
	 void saveFeatures(const Features& features) override;

	/**
	 * Store the names of the features.
	 */
	 void saveFeatureNames(const std::vector<std::string>& names) override;

	/**
	 * Store the given tube skeletons.
	 */
	void saveSkeletons(const Skeletons& skeletons) override;

	/**
	 * Store the given tube graph volumes.
	 */
	void saveGraphVolumes(const GraphVolumes& graphVolumes) override;

	/**
	 * Get all tube ids that this store offers.
	 */
	 TubeIds getTubeIds() override;

	/**
	 * Get the volumes for the given tube ids and store them in the given 
	 * property map. If onlyGeometry is true, only the bounding boxes and voxel 
	 * resolutions of the volumes are retrieved.
	 */
	 void retrieveVolumes(const TubeIds& ids, Volumes& volumes, bool onlyGeometry = false) override;

	/**
	 * Get the features for the given tube ids and store them in the given 
	 * property map.
	 */
	 void retrieveFeatures(const TubeIds& ids, Features& features) override;

	/**
	 * Get the skeletons for the given tube ids and store them in the given 
	 * property map.
	 */
	void retrieveSkeletons(const TubeIds& ids, Skeletons& skeletons) override;

	/**
	 * Get the skeletons for the given tube ids and store them in the given 
	 * property map.
	 */
	void retrieveGraphVolumes(const TubeIds& ids, GraphVolumes& graphVolumes) override;

private:

	/**
	 * Converts Position objects into array-like objects for HDF5 storage.
	 */
	struct PositionConverter {

		typedef float    ArrayValueType;
		static const int ArraySize = 3;

		vigra::ArrayVector<float> operator()(const Skeleton::Position& pos) const {

			vigra::ArrayVector<float> array(3);
			for (int i = 0; i < 3; i++)
				array[i] = pos[i];
			return array;
		}

		Skeleton::Position operator()(const vigra::ArrayVectorView<float>& array) const {

			Skeleton::Position pos;
			for (int i = 0; i < 3; i++)
				pos[i] = array[i];

			return pos;
		}

	};

	void writeGraphVolume(const GraphVolume& graphVolume);
	void readGraphVolume(GraphVolume& graphVolume);

	vigra::HDF5File _hdfFile;
};

#endif // HOST_TUBES_IO_HDF_TUBE_STORE_H__

