#ifndef HOST_TUBES_GUI_TUBE_VIEW_H__
#define HOST_TUBES_GUI_TUBE_VIEW_H__

#include <scopegraph/Scope.h>
#include "SkeletonView.h"
#include "NormalsView.h"
#include <sg_gui/MeshView.h>
#include <sg_gui/VolumeView.h>
#include <sg_gui/KeySignals.h>

class TubeView :
		public sg::Scope<
				TubeView,
				sg::Accepts<
						sg_gui::KeyDown
				>,
				sg::Provides<
						sg_gui::ContentChanged
				>,
				sg::ProvidesInner<
						sg_gui::ChangeAlpha
				>,
				sg::PassesUp<
						sg_gui::ContentChanged,
						sg_gui::VolumePointSelected
				>
		> {

public:

	TubeView();

	void setTubeMeshes(std::shared_ptr<sg_gui::Meshes> meshes);

	void setTubeSkeletons(std::shared_ptr<Skeletons> skeletons);

	void setRawVolume(std::shared_ptr<ExplicitVolume<float>> volume);

	void setLabelsVolume(std::shared_ptr<ExplicitVolume<float>> volume);

	void onSignal(sg_gui::KeyDown& signal);

private:

	/**
	 * Scope preventing change alpha signals to get to raw images.
	 */
	class RawScope : public sg::Scope<
			RawScope,
			sg::FiltersDown<
					sg_gui::ChangeAlpha,
					sg_gui::DrawOpaque
			>,
			sg::PassesUp<
					sg_gui::ContentChanged
			>
	> {

	public:

		RawScope() : _zBufferWrites(false) {}

		bool filterDown(sg_gui::ChangeAlpha&) { return false; }
		void unfilterDown(sg_gui::ChangeAlpha&) {}

		// disable z-write for the raw image
		bool filterDown(sg_gui::DrawOpaque&)   { if (!_zBufferWrites) glDepthMask(GL_FALSE); return true; }
		void unfilterDown(sg_gui::DrawOpaque&) { if (!_zBufferWrites) glDepthMask(GL_TRUE); }

		void toggleZBufferWrites() { _zBufferWrites = !_zBufferWrites; }

	private:

		bool _zBufferWrites;
	};

	/**
	 * Scope preventing change alpha signals to get to label images, also 
	 * ignores depth buffer for drawing on top of raw images.
	 */
	class LabelsScope : public sg::Scope<
			LabelsScope,
			sg::FiltersDown<
					sg_gui::DrawOpaque,
					sg_gui::DrawTranslucent,
					sg_gui::ChangeAlpha
			>,
			sg::AcceptsInner<
					sg::AgentAdded
			>,
			sg::ProvidesInner<
					sg_gui::ChangeAlpha,
					sg_gui::DrawTranslucent
			>,
			sg::PassesUp<
					sg_gui::ContentChanged,
					sg_gui::VolumePointSelected
			>
	> {

	public:

		LabelsScope() : _visible(true) {}

		void onInnerSignal(sg::AgentAdded&) {

			sendInner<sg_gui::ChangeAlpha>(0.5);
		}

		// stop the translucent draw -- we take care of it in opaque draw
		bool filterDown(sg_gui::DrawTranslucent&) { return false; }
		void unfilterDown(sg_gui::DrawTranslucent&) {}

		// convert opaque draw into translucent draw
		bool filterDown(sg_gui::DrawOpaque& s) {

				if (!_visible)
					return false;

				glEnable(GL_BLEND);
				sg_gui::DrawTranslucent signal;
				signal.roi() = s.roi();
				signal.resolution() = s.resolution();
				sendInner(signal);
				glDisable(GL_BLEND);
				
				return false;
		}
		void unfilterDown(sg_gui::DrawOpaque&) {}

		bool filterDown(sg_gui::ChangeAlpha&) { return false; }
		void unfilterDown(sg_gui::ChangeAlpha&) {}

		void toggleVisibility() {

			_visible = !_visible;
		}

	private:

		bool _visible;
	};


	std::shared_ptr<SkeletonView>       _skeletonView;
	std::shared_ptr<NormalsView>        _normalsView;
	std::shared_ptr<sg_gui::MeshView>   _meshView;
	std::shared_ptr<RawScope>           _rawScope;
	std::shared_ptr<LabelsScope>        _labelsScope;
	std::shared_ptr<sg_gui::VolumeView> _rawView;
	std::shared_ptr<sg_gui::VolumeView> _labelsView;

	double _alpha;
};

#endif // HOST_TUBES_GUI_TUBE_VIEW_H__

