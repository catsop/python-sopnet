/**
 * This program reads a label image stack and an intensity image stack and 
 * created an HDF5 project file that can be used by subsequent tools (like 
 * extract_tubes).
 */

#include <iostream>
#include <boost/filesystem.hpp>

#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/exceptions.h>
#include <pipeline/Process.h>
#include <pipeline/Value.h>
#include <imageprocessing/io/ImageStackDirectoryReader.h>
#include <volumes/io/Hdf5VolumeStore.h>
#include <volumes/ExtractLabels.h>

util::ProgramOption optionIntensities(
		util::_long_name        = "intensities",
		util::_description_text = "A directory containing the intensity volume.",
		util::_default_value    = "intensities");

util::ProgramOption optionLabels(
		util::_long_name        = "labels",
		util::_description_text = "A directory containing the labeled volume.",
		util::_default_value    = "labels");

util::ProgramOption optionExtractLabels(
		util::_long_name        = "extractLabels",
		util::_description_text = "Indicate that the labeled volume consists of a foreground/background labeling "
		                          "(dark/bright) and each 4-connected component of foreground represents one region.");

util::ProgramOption optionProjectFile(
		util::_long_name        = "projectFile",
		util::_short_name       = "p",
		util::_description_text = "The project file to store the label and intensity volumes.",
		util::_default_value    = "project.hdf");

int main(int argc, char** argv) {

	try {

		util::ProgramOptions::init(argc, argv);
		logger::LogManager::init();

		pipeline::Process<ImageStackDirectoryReader> intensityReader(optionIntensities.as<std::string>());
		pipeline::Process<ImageStackDirectoryReader> labelReader(optionLabels.as<std::string>());

		pipeline::Value<ImageStack> intensityStack = intensityReader->getOutput();
		pipeline::Value<ImageStack> labelStack = labelReader->getOutput();

		if (optionExtractLabels) {

			LOG_DEBUG(logger::out) << "[main] extracting labels from connected components" << std::endl;

			pipeline::Process<ExtractLabels> extractLabels;
			extractLabels->setInput(labelReader->getOutput());
			labelStack = extractLabels->getOutput();
		}

		unsigned int width  = labelStack->width();
		unsigned int height = labelStack->height();
		unsigned int depth  = labelStack->size();

		if (width  != intensityStack->width() ||
			height != intensityStack->height() ||
			depth  != intensityStack->size())
			UTIL_THROW_EXCEPTION(
					UsageError,
					"intensity and label stacks have different sizes");

		// create volumes from stacks

		ExplicitVolume<float> intensities(*intensityStack);
		ExplicitVolume<int>   labels(*labelStack);

		// store them in the project file

		boost::filesystem::remove(optionProjectFile.as<std::string>());
		Hdf5VolumeStore volumeStore(optionProjectFile.as<std::string>());

		volumeStore.saveIntensities(intensities);
		volumeStore.saveLabels(labels);

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
	}
}
