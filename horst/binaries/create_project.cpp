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
#include <sopnet/blockwise/persistence/BackendClient.h>

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

util::ProgramOption optionUseCatsopProject(
		util::_long_name        = "useCatsopProject",
		util::_description_text = "Read the intensities and labels from a CATSOP project. "
		                          "Set the catsop options accordingly.");

util::ProgramOption optionCatsopSegmentationId(
		util::_long_name        = "catsopSegmentationId",
		util::_description_text = "The segmentation id used by catsop to refer to a set of stacks and "
		                          "extracted structures.");

util::ProgramOption optionCatsopPgHost(
		util::_long_name        = "catsopPgHost",
		util::_description_text = "The postgresql host.",
		util::_default_value    = "localhost");

util::ProgramOption optionCatsopPgUser(
		util::_long_name        = "catsopPgUser",
		util::_description_text = "The postgresql username.",
		util::_default_value    = "catsop");

util::ProgramOption optionCatsopPgPassword(
		util::_long_name        = "catsopPgPassword",
		util::_description_text = "The postgresql password.",
		util::_default_value    = "catsop");

util::ProgramOption optionCatsopPgDatabase(
		util::_long_name        = "catsopPgDatabase",
		util::_description_text = "The postgresql database.",
		util::_default_value    = "catsop");

util::ProgramOption optionProjectFile(
		util::_long_name        = "projectFile",
		util::_short_name       = "p",
		util::_description_text = "The project file to store the label and intensity volumes.",
		util::_default_value    = "project.hdf");

int main(int argc, char** argv) {

	try {

		util::ProgramOptions::init(argc, argv);
		logger::LogManager::init();

		pipeline::Value<ImageStack> intensityStack;
		pipeline::Value<ImageStack> labelStack;

		if (optionUseCatsopProject) {

			StackDescription rawStackDescription;
			rawStackDescription.segmentationId = optionCatsopSegmentationId;
			StackDescription memStackDescription;
			memStackDescription.segmentationId = optionCatsopSegmentationId;

			ProjectConfiguration configuration;
			configuration.setBackendType(ProjectConfiguration::PostgreSql);
			configuration.setPostgreSqlHost(optionCatsopPgHost);
			configuration.setPostgreSqlUser(optionCatsopPgUser);
			configuration.setPostgreSqlPassword(optionCatsopPgPassword);
			configuration.setPostgreSqlDatabase(optionCatsopPgDatabase);
			configuration.setCatmaidStack(Raw, rawStackDescription);
			configuration.setCatmaidStack(Membrane, memStackDescription);

			BackendClient backendClient;
			backendClient.fillProjectConfiguration(configuration);

			// get the complete raw stack

			boost::shared_ptr<StackStore> stackStore =
					backendClient.createStackStore(configuration, Raw);

			util::box<unsigned int, 3> volumeBox(
					util::point<unsigned int, 3>(0, 0, 0),
					configuration.getVolumeSize());
			LOG_USER(logger::out) << "reading raw images in " << volumeBox << "..." << std::flush;
			intensityStack = stackStore->getImageStack(volumeBox);
			LOG_USER(logger::out) << " done." << std::endl;

			// DEBUG
			return 0;

		} else {

			pipeline::Process<ImageStackDirectoryReader> intensityReader(optionIntensities.as<std::string>());
			pipeline::Process<ImageStackDirectoryReader> labelReader(optionLabels.as<std::string>());

			intensityStack = intensityReader->getOutput();
			labelStack = labelReader->getOutput();
		}

		if (optionExtractLabels) {

			LOG_DEBUG(logger::out) << "[main] extracting labels from connected components" << std::endl;

			pipeline::Process<ExtractLabels> extractLabels;
			extractLabels->setInput(labelStack);
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
