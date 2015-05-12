#ifndef HOST_TUBES_FEATURE_EXTRACTOR_H__
#define HOST_TUBES_FEATURE_EXTRACTOR_H__

#include <util/assert.h>
#include <tubes/io/TubeStore.h>

class FeatureExtractor {

public:

	FeatureExtractor(TubeStore* store) :
		_store(store) {}

	/**
	 * Extract features for a set of tubes given in one label image. The values 
	 * of the label image are interpreted as tube ids. This version is more 
	 * efficient then calling extractFrom() for a set of volumes.
	 */
	void extractFrom(
			const ExplicitVolume<float>& intensities,
			const ExplicitVolume<int>&   labels);

	/**
	 * Extract features for a set of tubes given as individual volumes.
	 */
	void extractFrom(
			const ExplicitVolume<float>& intensities,
			const Volumes&               volumes);

private:

	TubeStore* _store;
};

#endif // HOST_TUBES_FEATURE_EXTRACTOR_H__

