#include <boost/lexical_cast.hpp>
#include <util/Logger.h>
#include "Hdf5TubeStore.h"

logger::LogChannel hdf5storelog("hdf5storelog", "[Hdf5TubeStore] ");

void
Hdf5TubeStore::saveVolumes(const Volumes& volumes) {

	_hdfFile.root();
	_hdfFile.cd_mk("tubes");
	_hdfFile.cd_mk("volumes");

	for (auto& p : volumes) {

		TubeId                               id     = p.first;
		const ExplicitVolume<unsigned char>& volume = p.second;
		std::string                          name   = boost::lexical_cast<std::string>(id);

		writeVolume(volume, name);
	}
}

void
Hdf5TubeStore::saveFeatures(const Features& features) {

	_hdfFile.root();
	_hdfFile.cd_mk("tubes");
	_hdfFile.cd_mk("features");

	for (auto& p : features) {

		TubeId                     id = p.first;
		const std::vector<double>& f  = p.second;

		_hdfFile.write(
				boost::lexical_cast<std::string>(id),
				vigra::ArrayVectorView<double>(f.size(), const_cast<double*>(&f[0])));
	}
}

void
Hdf5TubeStore::saveFeatureNames(const std::vector<std::string>& names) {

	_hdfFile.root();
	_hdfFile.cd_mk("tubes");
	_hdfFile.cd_mk("feature_names");

	for (const std::string& name : names)
		_hdfFile.mkdir(name);
}

void
Hdf5TubeStore::saveSkeletons(const Skeletons& skeletons) {

	_hdfFile.root();
	_hdfFile.cd_mk("tubes");
	_hdfFile.cd_mk("skeletons");

	for (auto& p : skeletons) {

		TubeId          id       = p.first;
		const Skeleton& skeleton = p.second;
		std::string     name     = boost::lexical_cast<std::string>(id);

		_hdfFile.cd_mk(name);

		writeGraphVolume(skeleton);
		writeNodeMap(skeleton.graph(), skeleton.diameters(), "diameters");

		_hdfFile.cd_up();
	}
}

void
Hdf5TubeStore::saveGraphVolumes(const GraphVolumes& graphVolumes) {

	_hdfFile.root();
	_hdfFile.cd_mk("tubes");
	_hdfFile.cd_mk("graph_volumes");

	for (auto& p : graphVolumes) {

		TubeId             id          = p.first;
		const GraphVolume& graphVolume = p.second;
		std::string        name        = boost::lexical_cast<std::string>(id);

		_hdfFile.cd_mk(name);

		writeGraphVolume(graphVolume);

		_hdfFile.cd_up();
	}
}

TubeIds
Hdf5TubeStore::getTubeIds() {

	_hdfFile.cd("/tubes/volumes");

	TubeIds ids;
	for (std::string& idString : _hdfFile.ls())
		ids.add(boost::lexical_cast<TubeId>(idString));

	return ids;
}

void
Hdf5TubeStore::retrieveVolumes(const TubeIds& ids, Volumes& volumes, bool onlyGeometry) {

	_hdfFile.cd("/tubes/volumes");

	for (TubeId id : ids) {

		std::string name = boost::lexical_cast<std::string>(id);

		ExplicitVolume<unsigned char> volume;

		readVolume(volume, name, onlyGeometry);

		volumes.insert(id, std::move(volume));
	}
}

void
Hdf5TubeStore::retrieveFeatures(const TubeIds&, Features&) {

}

void
Hdf5TubeStore::retrieveSkeletons(const TubeIds& ids, Skeletons& skeletons) {

	_hdfFile.cd("/tubes/skeletons");

	for (TubeId id : ids) {

		LOG_ALL(hdf5storelog) << "reading skeleton for tube " << id << std::endl;

		std::string name = boost::lexical_cast<std::string>(id);

		_hdfFile.cd(name);

		Skeleton skeleton;
		readGraphVolume(skeleton);
		readNodeMap(skeleton.graph(), skeleton.diameters(), "diameters");
		skeletons.insert(id, std::move(skeleton));

		_hdfFile.cd_up();
	}
}

void
Hdf5TubeStore::retrieveGraphVolumes(const TubeIds& ids, GraphVolumes& graphVolumes) {

	_hdfFile.cd("/tubes/graph_volumes");

	for (TubeId id : ids) {

		LOG_ALL(hdf5storelog) << "reading graph volume for tube " << id << std::endl;

		std::string name = boost::lexical_cast<std::string>(id);

		_hdfFile.cd(name);

		GraphVolume graphVolume;
		readGraphVolume(graphVolume);
		graphVolumes.insert(id, std::move(graphVolume));

		_hdfFile.cd_up();
	}
}

void
Hdf5TubeStore::writeGraphVolume(const GraphVolume& graphVolume) {

	PositionConverter positionConverter;

	writeGraph(graphVolume.graph());
	writeNodeMap(graphVolume.graph(), graphVolume.positions(), "positions", positionConverter);

	vigra::MultiArray<1, float> p(3);

	// resolution
	p[0] = graphVolume.getResolutionX();
	p[1] = graphVolume.getResolutionY();
	p[2] = graphVolume.getResolutionZ();
	_hdfFile.write("resolution", p);

	// offset
	p[0] = graphVolume.getOffset().x();
	p[1] = graphVolume.getOffset().y();
	p[2] = graphVolume.getOffset().z();
	_hdfFile.write("offset", p);
}

void
Hdf5TubeStore::readGraphVolume(GraphVolume& graphVolume) {

	PositionConverter positionConverter;

	readGraph(graphVolume.graph());
	readNodeMap(graphVolume.graph(), graphVolume.positions(), "positions", positionConverter);

	vigra::MultiArray<1, float> p(3);

	// resolution
	_hdfFile.read("resolution", p);
	graphVolume.setResolution(p[0], p[1], p[2]);

	// offset
	_hdfFile.read("offset", p);
	graphVolume.setOffset(p[0], p[1], p[2]);
}
