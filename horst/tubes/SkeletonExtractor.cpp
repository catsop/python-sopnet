#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include "SkeletonExtractor.h"
#include "Skeletons.h"
#include "GraphVolumes.h"
#define WITH_LEMON
#include <vigra/multi_impex.hxx>
#include <vigra/multi_resize.hxx>
#include <vigra/multi_watersheds.hxx>
#include <vigra/functorexpression.hxx>
#include <util/timing.h>

logger::LogChannel skeletonextractorlog("skeletonextractorlog", "[SkeletonExtractor] ");

util::ProgramOption optionSkeletonDownsampleVolume(
		util::_long_name        = "skeletonDownsampleVolume",
		util::_description_text = "downsample the volume dimensions by the largest power of two that does not change connectivity.");

void
SkeletonExtractor::extract() {

	bool downsample = optionSkeletonDownsampleVolume;

	TubeIds ids = _store->getTubeIds();

	Volumes volumes;
	_store->retrieveVolumes(ids, volumes);

	Skeletons    skeletons;
	GraphVolumes graphVolumes;

	for (TubeId id : ids) {

		LOG_DEBUG(skeletonextractorlog)
				<< "processing tube " << id << std::endl;

		try {

			Timer t("skeletonize volume");

			ExplicitVolume<float> downsampled;

			if (downsample)
				downsampled = downsampleVolume(volumes[id]);
			else
				downsampled = volumes[id];

			LOG_DEBUG(skeletonextractorlog)
					<< "original volume has discrete bb " << volumes[id].getDiscreteBoundingBox()
					<< ", offset " << volumes[id].getOffset() << ", and resolution " << volumes[id].getResolution()
					<< std::endl;

			LOG_DEBUG(skeletonextractorlog)
					<< "downsampled volume has discrete bb " << downsampled.getDiscreteBoundingBox()
					<< ", offset " << downsampled.getOffset() << ", and resolution " << downsampled.getResolution()
					<< std::endl;

			GraphVolume graph(downsampled);

			LOG_DEBUG(skeletonextractorlog)
					<< "graph volume has discrete bb " << graph.getDiscreteBoundingBox()
					<< ", offset " << graph.getOffset() << ", and resolution " << graph.getResolution()
					<< std::endl;

			Skeletonize skeletonize(graph);

			skeletons.insert(id, skeletonize.getSkeleton());
			graphVolumes.insert(id, std::move(graph));

		} catch (NoNodeFound& e) {

			LOG_USER(skeletonextractorlog)
					<< "tube " << id
					<< " could not be skeletonized (NoNodeFound)"
					<< std::endl;
		}
	}

	_store->saveSkeletons(skeletons);
	_store->saveGraphVolumes(graphVolumes);
}


ExplicitVolume<float>
SkeletonExtractor::downsampleVolume(const ExplicitVolume<unsigned char>& volume) {

	vigra::TinyVector<float, 3> origRes = {
			volume.getResolutionX(),
			volume.getResolutionY(),
			volume.getResolutionZ()};


	vigra::TinyVector<int, 3> origSize = {
			(int)volume.width(),
			(int)volume.height(),
			(int)volume.depth()};

	float finestRes = -1;
	int   finestDimension;
	for (int d = 0; d < 3; d++)
		if (finestRes < 0 || finestRes > origRes[d]) {

			finestRes = origRes[d];
			finestDimension = d;
		}

	// the largest downsample factor to consider
	int downsampleFactor = 8;

	while (true) {

		LOG_DEBUG(skeletonextractorlog)
				<< "trying to downsample finest dimension by factor "
				<< downsampleFactor << std::endl;

		vigra::TinyVector<int, 3>   factors;
		vigra::TinyVector<float, 3> targetRes;
		vigra::TinyVector<int, 3>   targetSize;

		factors[finestDimension]    = downsampleFactor;
		targetRes[finestDimension]  = origRes[finestDimension]*downsampleFactor;
		targetSize[finestDimension] = origSize[finestDimension]/downsampleFactor;

		// the target resolution of the finest dimension, when downsampled with 
		// current factor
		float targetFinestRes = finestRes*downsampleFactor;

		// for each other dimension, find best downsample factor
		for (int d = 0; d < 3; d++) {

			if (d == finestDimension)
				continue;

			int bestFactor = 0;
			float minResDiff = 0;

			for (int f = downsampleFactor; f != 0; f /= 2) {

				float targetRes = origRes[d]*f;
				float resDiff = std::abs(targetFinestRes - targetRes);

				if (bestFactor == 0 || resDiff < minResDiff) {

					bestFactor = f;
					minResDiff = resDiff;
				}
			}

			factors[d]    = bestFactor;
			targetRes[d]  = origRes[d]*bestFactor;
			targetSize[d] = origSize[d]/bestFactor;
		}

		LOG_DEBUG(skeletonextractorlog)
				<< "best downsampling factors for each dimension are "
				<< factors << std::endl;

		ExplicitVolume<unsigned char> downsampled(targetSize[0], targetSize[1], targetSize[2]);
		downsampled.setResolution(targetRes[0], targetRes[1], targetRes[2]);
		downsampled.setOffset(volume.getOffset());

		// copy volume
		for (int z = 0; z < targetSize[2]; z++)
		for (int y = 0; y < targetSize[1]; y++)
		for (int x = 0; x < targetSize[0]; x++)
			downsampled(x, y, z) = volume(x*factors[0], y*factors[1], z*factors[2]);

		int numRegions;
		try {

			// check for downsampling errors
			vigra::MultiArray<3, unsigned int> labels(downsampled.data().shape());
			numRegions = vigra::labelMultiArrayWithBackground(
					downsampled.data(),
					labels);

		} catch (vigra::InvariantViolation& e) {

			LOG_DEBUG(skeletonextractorlog)
					<< "downsampled image contains more than 255 connected components"
					<< std::endl;

			numRegions = 2;
		}

		LOG_DEBUG(skeletonextractorlog)
				<< "downsampled image contains " << numRegions
				<< " connected components" << std::endl;

		if (numRegions == 1)
			return downsampled;

		downsampleFactor /= 2;
		if (downsampleFactor == 1)
			return volume;
	}

}
