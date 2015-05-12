#ifndef HOST_TUBES_SKELETONS_H__
#define HOST_TUBES_SKELETONS_H__

#include <imageprocessing/Volume.h>
#include <imageprocessing/Skeleton.h>
#include "TubePropertyMap.h"

class Skeletons : public TubePropertyMap<Skeleton>, public Volume {

protected:

	util::box<float,3> computeBoundingBox() const override {

		util::box<float,3> bb;

		for (auto& p : *this)
			bb += p.second.getBoundingBox();

		return bb;
	}
};

#endif // HOST_TUBES_SKELETONS_H__

