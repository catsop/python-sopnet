#ifndef HOST_VOLUMES_IO_VOLUME_STORE_H__
#define HOST_VOLUMES_IO_VOLUME_STORE_H__

#include <imageprocessing/ExplicitVolume.h>

/**
 * Interface definition for volume stores.
 */
class VolumeStore {

public:

	/**
	 * Store the given intensity volume.
	 */
	virtual void saveIntensities(const ExplicitVolume<float>& intensities) = 0;

	/**
	 * Store the given label volume.
	 */
	virtual void saveLabels(const ExplicitVolume<int>& labels) = 0;

	/**
	 * Get the intensity volume.
	 */
	virtual void retrieveIntensities(ExplicitVolume<float>& intensities) = 0;

	/**
	 * Get the label volume.
	 */
	virtual void retrieveLabels(ExplicitVolume<int>& labels) = 0;
};


#endif // HOST_VOLUMES_IO_VOLUME_STORE_H__

