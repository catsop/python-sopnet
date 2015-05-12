#include "TubeView.h"
#include <util/ProgramOptions.h>

util::ProgramOption optionShowNormals(
		util::_long_name        = "showNormals",
		util::_description_text = "Show the mesh normals.");

TubeView::TubeView() :
	_skeletonView(std::make_shared<SkeletonView>()),
	_normalsView(std::make_shared<NormalsView>()),
	_meshView(std::make_shared<sg_gui::MeshView>()),
	_rawScope(std::make_shared<RawScope>()),
	_labelsScope(std::make_shared<LabelsScope>()),
	_rawView(std::make_shared<sg_gui::VolumeView>()),
	_labelsView(std::make_shared<sg_gui::VolumeView>()),
	_alpha(1.0) {

	_rawScope->add(_rawView);
	_labelsScope->add(_labelsView);

	add(_rawScope);
	add(_labelsScope);
	add(_skeletonView);
	add(_meshView);

	if (optionShowNormals)
		add(_normalsView);
}

void
TubeView::setTubeMeshes(std::shared_ptr<sg_gui::Meshes> meshes) {

	_normalsView->setMeshes(meshes);
	_meshView->setMeshes(meshes);
}

void
TubeView::setTubeSkeletons(std::shared_ptr<Skeletons> skeletons) {

	_skeletonView->setSkeletons(skeletons);
}

void
TubeView::setRawVolume(std::shared_ptr<ExplicitVolume<float>> volume) {

	_rawView->setVolume(volume);
}

void
TubeView::setLabelsVolume(std::shared_ptr<ExplicitVolume<float>> volume) {

	_labelsView->setVolume(volume);
}

void
TubeView::onSignal(sg_gui::KeyDown& signal) {

	if (signal.key == sg_gui::keys::Tab) {

		_alpha += 0.5;
		if (_alpha > 1.0)
			_alpha = 0;

		sendInner<sg_gui::ChangeAlpha>(_alpha);
	}

	if (signal.key == sg_gui::keys::L) {

		_rawScope->toggleZBufferWrites();
		_labelsScope->toggleVisibility();
		send<sg_gui::ContentChanged>();
	}
}
