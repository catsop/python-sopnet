#ifndef HOST_TUBES_GUI_MESH_VIEW_CONTROLLER_H__
#define HOST_TUBES_GUI_MESH_VIEW_CONTROLLER_H__

#include <tubes/io/TubeStore.h>
#include <scopegraph/Agent.h>
#include <sg_gui/VolumeView.h>
#include <sg_gui/MeshView.h>
#include <sg_gui/Meshes.h>

template <typename EV>
class ExplicitVolumeAdaptor {

public:

	typedef typename EV::value_type value_type;

	ExplicitVolumeAdaptor(const EV& ev) :
		_ev(ev) {}

	const util::box<float,3>& getBoundingBox() const { return _ev.getBoundingBox(); }

	float operator()(float x, float y, float z) const {

		if (!getBoundingBox().contains(x, y, z))
			return 0;

		unsigned int dx, dy, dz;

		_ev.getDiscreteCoordinates(x, y, z, dx, dy, dz);

		return _ev(dx, dy, dz);
	}

private:

	const EV& _ev;

};

class MeshViewController :
		public sg::Agent<
				MeshViewController,
				sg::Accepts<
						sg_gui::VolumePointSelected
				>,
				sg::Provides<
						sg_gui::SetMeshes
				>
		> {

public:

	MeshViewController(
			TubeStore*                             tubeStore,
			std::shared_ptr<ExplicitVolume<float>> labels);

	void loadMeshes(TubeIds ids);

	void onSignal(sg_gui::VolumePointSelected& signal);

private:

	void addMesh(TubeId id);

	void removeMesh(TubeId id);

	TubeStore* _tubeStore;

	std::shared_ptr<ExplicitVolume<float>> _labels;

	std::shared_ptr<sg_gui::Meshes> _meshes;
};

#endif // HOST_TUBES_GUI_MESH_VIEW_CONTROLLER_H__

