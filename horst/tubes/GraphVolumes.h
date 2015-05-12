#ifndef HOST_TUBES_GRAPH_VOLUMES_H__
#define HOST_TUBES_GRAPH_VOLUMES_H__

#include <imageprocessing/Volume.h>
#include <imageprocessing/GraphVolume.h>
#include "TubePropertyMap.h"

class GraphVolumes : public TubePropertyMap<GraphVolume>, public Volume {

protected:

	util::box<float,3> computeBoundingBox() const override {

		util::box<float,3> bb;

		for (auto& p : *this)
			bb += p.second.getBoundingBox();

		return bb;
	}
};

#endif // HOST_TUBES_GRAPH_VOLUMES_H__


