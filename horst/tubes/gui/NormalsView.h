#ifndef HOST_TUBUS_GUI_NORMALS_VIEW_H__
#define HOST_TUBUS_GUI_NORMALS_VIEW_H__

#include <scopegraph/Agent.h>
#include <sg_gui/Meshes.h>
#include <sg_gui/RecordableView.h>
#include <sg_gui/GuiSignals.h>

class NormalsView :
		public sg::Agent<
				NormalsView,
				sg::Accepts<
						sg_gui::Draw,
						sg_gui::QuerySize
				>,
				sg::Provides<
						sg_gui::ContentChanged
				>
		>,
		public sg_gui::RecordableView {

public:

	void setMeshes(std::shared_ptr<sg_gui::Meshes> meshes);

	void onSignal(sg_gui::Draw& draw);

	void onSignal(sg_gui::QuerySize& signal);

private:

	void updateRecording();

	std::shared_ptr<sg_gui::Meshes> _meshes;
};

#endif // HOST_TUBUS_GUI_NORMALS_VIEW_H__

