#ifndef HOST_TUBES_GUI_SKELETON_VIEW_H__
#define HOST_TUBES_GUI_SKELETON_VIEW_H__

#include <scopegraph/Agent.h>
#include <tubes/Skeletons.h>
#include <sg_gui/GuiSignals.h>
#include <sg_gui/MouseSignals.h>
#include <sg_gui/RecordableView.h>
#include <sg_gui/Sphere.h>

class SetSkeletons : public sg_gui::SetContent {

public:

	typedef sg_gui::SetContent parent_type;

	SetSkeletons(std::shared_ptr<Skeletons> skeletons) :
			_skeletons(skeletons) {}

	std::shared_ptr<Skeletons> getSkeletons() { return _skeletons; }

private:

	std::shared_ptr<Skeletons> _skeletons;
};

class SkeletonView :
		public sg::Agent<
				SkeletonView,
				sg::Accepts<
						sg_gui::Draw,
						sg_gui::QuerySize,
						sg_gui::MouseDown,
						SetSkeletons
				>,
				sg::Provides<
						sg_gui::ContentChanged
				>
		>,
		public sg_gui::RecordableView {

public:

	SkeletonView();

	void setSkeletons(std::shared_ptr<Skeletons> skeletons);

	void onSignal(sg_gui::Draw& draw);

	void onSignal(sg_gui::QuerySize& signal);

	void onSignal(sg_gui::MouseDown& signal);

	void onSignal(SetSkeletons& signal);

private:

	void updateRecording();

	void drawSkeleton(const Skeleton& skeleton);

	void drawSphere(const util::point<float,3>& center, float diameter);

	std::shared_ptr<Skeletons> _skeletons;

	sg_gui::Sphere _sphere;

	float _sphereScale;
};

#endif // HOST_TUBES_GUI_SKELETON_VIEW_H__

