#include "SkeletonViewController.h"

SkeletonViewController::SkeletonViewController(
		TubeStore* tubeStore,
		std::shared_ptr<ExplicitVolume<float>> labels) :
	_tubeStore(tubeStore),
	_labels(labels),
	_skeletons(std::make_shared<Skeletons>()) {}

void
SkeletonViewController::loadSkeletons(TubeIds ids) {

	for (TubeId id : ids)
		addSkeleton(id);

	send<SetSkeletons>(_skeletons);
}

void
SkeletonViewController::onSignal(sg_gui::VolumePointSelected& signal) {

	unsigned int x, y, z;
	_labels->getDiscreteCoordinates(
			signal.position().x(),
			signal.position().y(),
			signal.position().z(),
			x, y, z);
	TubeId id = (*_labels)(x, y, z);

	if (id == 0)
		return;

	if (_skeletons->contains(id))
		removeSkeleton(id);
	else
		addSkeleton(id);

	send<SetSkeletons>(_skeletons);
}

void
SkeletonViewController::addSkeleton(TubeId id) {

	TubeIds ids;
	ids.add(id);

	Skeletons skeletons;
	_tubeStore->retrieveSkeletons(ids, skeletons);
	_skeletons->insert(id, std::move(skeletons[id]));
}

void
SkeletonViewController::removeSkeleton(TubeId id) {

	_skeletons->remove(id);
}
