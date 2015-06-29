#include <util/timing.h>
#include <util/Logger.h>
#include <vigra/functorexpression.hxx>
#include "TubeExtractor.h"

logger::LogChannel tubeextractorlog("tubeextractorlog", "[TubeExtractor] ");

void
TubeExtractor::extractFrom(ExplicitVolume<int>& labels, const std::set<TubeId>& ids) {

	std::map<TubeId, util::box<float,3>> bbs;

	{
		UTIL_TIME_SCOPE("computing bounding boxes");

		// get the discrete bounding boxes of all tubes
		for (unsigned int z = 0; z < labels.depth();  z++)
		for (unsigned int y = 0; y < labels.height(); y++)
		for (unsigned int x = 0; x < labels.width();  x++) {

			TubeId id = labels(x, y, z);

			if (id == 0)
				continue;

			bbs[id] += util::box<float,3>(x, y, z, x+1, y+1, z+1);
		}
	}

	unsigned int numTubes = bbs.size();

	LOG_USER(tubeextractorlog) << "found " << numTubes << " tubes" << std::endl;

	unsigned int i = 0;
	for (auto& p : bbs) {

		if (i % 10 == 0)
		LOG_USER(tubeextractorlog)
				<< logger::delline << "extracting volumes... "
				<< ((float)i/numTubes)*100 << "%"
				<< std::flush;

		TubeId                    id = p.first;
		const util::box<float,3>& bb = p.second;

		LOG_DEBUG(tubeextractorlog) << "extracting tube " << id << std::endl;

		if (!ids.empty())
			if (!ids.count(id))
				continue;

		ExplicitVolume<unsigned char> volume(bb.width(), bb.height(), bb.depth());

		// set volume properties
		volume.setResolution(
				labels.getResolutionX(),
				labels.getResolutionY(),
				labels.getResolutionZ());
		volume.setOffset(labels.getOffset() + bb.min()*labels.getResolution());

		{
			UTIL_TIME_SCOPE("extract tube volume");

			// copy data
			vigra::transformMultiArray(
					labels.data().subarray(
							vigra::Shape3(bb.min().x(), bb.min().y(), bb.min().z()),
							vigra::Shape3(bb.max().x(), bb.max().y(), bb.max().z())),
					volume.data(),
					(vigra::functor::Arg1() == vigra::functor::Param(id)));
		}

		{
			UTIL_TIME_SCOPE("save tube volume");

			_store->saveVolume(id, volume);
		}
	}
}
