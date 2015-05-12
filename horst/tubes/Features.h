#ifndef HOST_FEATURES_FEATURES_H__
#define HOST_FEATURES_FEATURES_H__

#include <vector>
#include "TubePropertyMap.h"
#include "TubeIds.h"

class Features : public TubePropertyMap<std::vector<double>> {

public:

	/**
	 * Create an empty feature map.
	 */
	Features() {}

	/**
	 * Create a new feature map and reserve enough memory to fit the given 
	 * number of features for the given tube ids.
	 */
	Features(const TubeIds& ids, std::size_t numFeatures) {

		for (auto id : ids)
			(*this)[id].reserve(numFeatures);
	}

	/**
	 * Add a single feature to the feature vector for a tube id. Converts nan 
	 * into 0.
	 */
	inline void append(TubeId id, double feature) {

		if (feature != feature)
			feature = 0;

		(*this)[id].push_back(feature);
	}
};

#endif // HOST_FEATURES_FEATURES_H__

