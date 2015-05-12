/**
 * This program reads tubes from an HDF5 file, computes their features, and 
 * stores the result in the same file.
 */

#include <iostream>

#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/exceptions.h>
#include <region_features/RegionFeatures.h>
#include <volumes/io/Hdf5VolumeStore.h>
#include <tubes/io/Hdf5TubeStore.h>
#include <tubes/FeatureExtractor.h>
#include <tubes/SkeletonExtractor.h>

util::ProgramOption optionProjectFile(
		util::_long_name        = "projectFile",
		util::_short_name       = "p",
		util::_description_text = "The project file to read the label and intensity volume and store the features for each tube.",
		util::_default_value    = "project.hdf");

util::ProgramOption optionSkeletonsOnly(
		util::_long_name        = "skeletonsOnly",
		util::_description_text = "Don't extract other features, only the skeletons of the tubes.");

int main(int argc, char** argv) {

	try {

		util::ProgramOptions::init(argc, argv);
		logger::LogManager::init();

		// read the label and intensity volumes

		ExplicitVolume<float> intensities;
		ExplicitVolume<int>   labels;

		Hdf5VolumeStore volumeStore(optionProjectFile.as<std::string>());
		volumeStore.retrieveIntensities(intensities);
		volumeStore.retrieveLabels(labels);

		// create an hdf5 tube store

		Hdf5TubeStore tubeStore(optionProjectFile.as<std::string>());

		if (!optionSkeletonsOnly) {

			// extract and save tube features

			LOG_USER(logger::out) << "extracting features..." << std::endl;

			FeatureExtractor featureExtractor(&tubeStore);
			featureExtractor.extractFrom(intensities, labels);
		}

		// extract and save tube skeletons

		LOG_USER(logger::out) << "extracting skeletons..." << std::endl;

		SkeletonExtractor skeletonExtractor(&tubeStore);
		skeletonExtractor.extract();

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
	}
}

