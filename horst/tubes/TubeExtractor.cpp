#include <vigra/functorexpression.hxx>
#include "TubeExtractor.h"

void
TubeExtractor::extractFrom(ExplicitVolume<int>& labels, const std::set<TubeId>& ids) {

	std::map<TubeId, util::box<float,3>> bbs;

	// get the discrete bounding boxes of all tubes
	for (unsigned int z = 0; z < labels.depth();  z++)
	for (unsigned int y = 0; y < labels.height(); y++)
	for (unsigned int x = 0; x < labels.width();  x++) {

		TubeId id = labels(x, y, z);

		if (id == 0)
			continue;

		bbs[id] += util::box<float,3>(x, y, z, x+1, y+1, z+1);
	}

	// extract the tube volumes
	Volumes volumes;

	for (auto& p : bbs) {

		TubeId              id = p.first;
		util::box<float,3>& bb = p.second;

		if (!ids.empty())
			if (!ids.count(id))
				continue;

		// set volume properties
		volumes[id].setResolution(
				labels.getResolutionX(),
				labels.getResolutionY(),
				labels.getResolutionZ());
		volumes[id].setOffset(labels.getOffset() + bb.min()*labels.getResolution());
		volumes[id].data() = vigra::MultiArray<3, char>(vigra::Shape3(bb.width(), bb.height(), bb.depth()));

		// copy data
		vigra::transformMultiArray(
				labels.data().subarray(
						vigra::Shape3(bb.min().x(), bb.min().y(), bb.min().z()),
						vigra::Shape3(bb.max().x(), bb.max().y(), bb.max().z())),
				volumes[id].data(),
				(vigra::functor::Arg1() == vigra::functor::Param(id)));
	}

	// save them
	_store->saveVolumes(volumes);
}
