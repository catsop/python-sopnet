#ifndef HOST_TUBES_IO_TUBE_STORE_H__
#define HOST_TUBES_IO_TUBE_STORE_H__

#include "Volumes.h"
#include "Features.h"
#include "Skeletons.h"
#include "GraphVolumes.h"

/**
 * Interface definition for tube stores.
 */
class TubeStore {

public:

	/**
	 * Store the given tube volumes.
	 */
	virtual void saveVolumes(const Volumes& volumes) = 0;

	/**
	 * Store the given tube features.
	 */
	virtual void saveFeatures(const Features& features) = 0;

	/**
	 * Store the names of the features.
	 */
	virtual void saveFeatureNames(const std::vector<std::string>& names) = 0;

	/**
	 * Store the given tube skeletons.
	 */
	virtual void saveSkeletons(const Skeletons& skeletons) = 0;

	/**
	 * Store the given tube graph volumes.
	 */
	virtual void saveGraphVolumes(const GraphVolumes& graphVolumes) = 0;

	/**
	 * Get all tube ids that this store offers.
	 */
	virtual TubeIds getTubeIds() = 0;

	/**
	 * Get the volumes for the given tube ids and store them in the given 
	 * property map. If onlyGeometry is true, only the bounding boxes and voxel 
	 * resolutions of the volumes are retrieved.
	 */
	virtual void retrieveVolumes(const TubeIds& ids, Volumes& volumes, bool onlyGeometry = false) = 0;

	/**
	 * Get the features for the given tube ids and store them in the given 
	 * property map.
	 */
	virtual void retrieveFeatures(const TubeIds& ids, Features& features) = 0;

	/**
	 * Get the skeletons for the given tube ids and store them in the given 
	 * property map.
	 */
	virtual void retrieveSkeletons(const TubeIds& ids, Skeletons& skeletons) = 0;

	/**
	 * Get the skeletons for the given tube ids and store them in the given 
	 * property map.
	 */
	virtual void retrieveGraphVolumes(const TubeIds& ids, GraphVolumes& graphVolumes) = 0;
};

#endif // HOST_TUBES_IO_TUBE_STORE_H__

