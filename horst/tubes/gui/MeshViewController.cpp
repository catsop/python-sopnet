#include "MeshViewController.h"
#include <sg_gui/MarchingCubes.h>
#include <util/ProgramOptions.h>

util::ProgramOption optionCubeSize(
		util::_long_name        = "cubeSize",
		util::_description_text = "The size of a cube for the marching cubes visualization.",
		util::_default_value    = 10);

MeshViewController::MeshViewController(
		TubeStore* tubeStore,
		std::shared_ptr<ExplicitVolume<float>> labels) :
	_tubeStore(tubeStore),
	_labels(labels),
	_meshes(std::make_shared<sg_gui::Meshes>()) {}

void
MeshViewController::loadMeshes(TubeIds ids) {

	for (TubeId id : ids)
		addMesh(id);

	send<sg_gui::SetMeshes>(_meshes);
}

void
MeshViewController::onSignal(sg_gui::VolumePointSelected& signal) {

	unsigned int x, y, z;
	_labels->getDiscreteCoordinates(
			signal.position().x(),
			signal.position().y(),
			signal.position().z(),
			x, y, z);
	TubeId id = (*_labels)(x, y, z);

	if (id == 0)
		return;

	if (_meshes->contains(id))
		removeMesh(id);
	else
		addMesh(id);

	send<sg_gui::SetMeshes>(_meshes);
}

void
MeshViewController::addMesh(TubeId id) {

	TubeIds ids;
	ids.add(id);

	// get tube volume

	Volumes volumes;
	_tubeStore->retrieveVolumes(ids, volumes);

	// volume -> mesh

	for (auto& p : volumes) {

		TubeId id                             = p.first;
		ExplicitVolume<unsigned char>& volume = p.second;

		typedef ExplicitVolumeAdaptor<ExplicitVolume<unsigned char>> Adaptor;
		Adaptor adaptor(volume);

		sg_gui::MarchingCubes<Adaptor> marchingCubes;
		std::shared_ptr<sg_gui::Mesh> mesh = marchingCubes.generateSurface(
				adaptor,
				sg_gui::MarchingCubes<Adaptor>::AcceptAbove(0.5),
				optionCubeSize,
				optionCubeSize,
				optionCubeSize);
		_meshes->add(id, mesh);
	}
}

void
MeshViewController::removeMesh(TubeId id) {

	_meshes->remove(id);
}
