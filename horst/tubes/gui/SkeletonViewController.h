#ifndef HOST_TUBES_GUI_SKELETON_VIEW_CONTROLLER_H__
#define HOST_TUBES_GUI_SKELETON_VIEW_CONTROLLER_H__

#include <tubes/io/TubeStore.h>
#include <scopegraph/Agent.h>
#include <sg_gui/VolumeView.h>
#include "SkeletonView.h"

class SkeletonViewController :
		public sg::Agent<
				SkeletonViewController,
				sg::Accepts<
						sg_gui::VolumePointSelected
				>,
				sg::Provides<
						SetSkeletons
				>
		> {

public:

	SkeletonViewController(
			TubeStore*                             tubeStore,
			std::shared_ptr<ExplicitVolume<float>> labels);

	void loadSkeletons(TubeIds ids);

	void onSignal(sg_gui::VolumePointSelected& signal);

private:

	void addSkeleton(TubeId id);

	void removeSkeleton(TubeId id);

	TubeStore* _tubeStore;

	std::shared_ptr<ExplicitVolume<float>> _labels;

	std::shared_ptr<Skeletons> _skeletons;
};

#endif // HOST_TUBES_GUI_SKELETON_VIEW_CONTROLLER_H__

