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
#include <util/timing.h>
#include <pipeline/Process.h>
#include <pipeline/Value.h>
#include <imageprocessing/io/ImageStackDirectoryReader.h>
#include <volumes/io/Hdf5VolumeStore.h>
#include <volumes/ExtractLabels.h>
#include <sopnet/blockwise/blocks/BlockUtils.h>
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

util::ProgramOption optionCatsopSegmentationConfigurationId(
		util::_long_name        = "catsopSegmentationConfigurationId",
		util::_description_text = "The segmentation configuration id used by catsop to refer to a set of stacks and "
		                          "extracted structures.");

util::ProgramOption optionCatsopComponentDirectory(
		util::_long_name        = "catsopComponentDirectory",
		util::_description_text = "The local directory where the slice images are stored.",
		util::_default_value    = "/tmp/catsop/");

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

		// create volumes from stacks

		ExplicitVolume<float> intensities;
		ExplicitVolume<int>   labels;

		if (optionUseCatsopProject) {

			ProjectConfiguration configuration;
			configuration.setBackendType(ProjectConfiguration::PostgreSql);
			configuration.setPostgreSqlHost(optionCatsopPgHost);
			configuration.setPostgreSqlUser(optionCatsopPgUser);
			configuration.setPostgreSqlPassword(optionCatsopPgPassword);
			configuration.setPostgreSqlDatabase(optionCatsopPgDatabase);
			configuration.setSegmentationConfigurationId(optionCatsopSegmentationConfigurationId);
			configuration.setComponentDirectory(optionCatsopComponentDirectory);

			BackendClient backendClient;
			backendClient.fillProjectConfiguration(configuration);

			// get the complete raw stack

			boost::shared_ptr<StackStore> stackStore =
					backendClient.createStackStore(configuration, Raw);

			util::box<unsigned int, 3> volumeBox(
					util::point<unsigned int, 3>(0, 0, 0),
					configuration.getVolumeSize());
			LOG_USER(logger::out) << "reading raw images in " << volumeBox << "..." << std::flush;
			{
				UTIL_TIME_SCOPE("read raw images");
				intensityStack = stackStore->getImageStack(volumeBox);
			}
			LOG_USER(logger::out) << " done." << std::endl;

			// create a intensity and label stack of the same size
			{
				UTIL_TIME_SCOPE("convert raw stack");
				intensities = ExplicitVolume<float>(*intensityStack);
			}
			labels = ExplicitVolume<int>(intensityStack->width(), intensityStack->height(), intensityStack->size());
			labels.data() = 0;

			// get all assemblies

			boost::shared_ptr<SegmentStore> segmentStore =
					backendClient.createSegmentStore(configuration);

			BlockUtils blockUtils(configuration);
			Cores cores;
			Core maxCore = blockUtils.getCoreAtLocation(configuration.getVolumeSize() - util::point<int, 3>(1, 1, 1));
			for (unsigned int z = 0; z <= maxCore.z(); z++)
			for (unsigned int y = 0; y <= maxCore.y(); y++)
			for (unsigned int x = 0; x <= maxCore.x(); x++)
				cores.add(Core(x, y, z));

			std::vector<std::set<SegmentHash>> assemblies;

			{
				UTIL_TIME_SCOPE("get assemblies");
				assemblies = segmentStore->getSolutionByCores(cores);
			}

			LOG_USER(logger::out) << "found " << assemblies.size() << " assemblies" << std::endl;

			// draw assemblies in labels volume

			LOG_USER(logger::out) << "creating label volume..." << std::flush;

			{
				UTIL_TIME_SCOPE("create label volume");

				unsigned int tubeId = 1;
				#pragma omp parallel for
				for (unsigned int i = 0; i < assemblies.size(); i++) {

					boost::shared_ptr<SliceStore> sliceStore = backendClient.createSliceStore(configuration);

					// get slices
					boost::shared_ptr<Slices> slices = sliceStore->getSlicesBySegmentHashes(assemblies[i]);

					// draw slices
					for (boost::shared_ptr<Slice> slice : *slices) {

						int z = slice->getSection();
						for (const util::point<unsigned int, 2>& p : slice->getComponent()->getPixels())
							labels(p.x(), p.y(), z) = tubeId;
					}

					tubeId++;
				}
			}

			LOG_USER(logger::out) << " done." << std::endl;

		} else {

			pipeline::Process<ImageStackDirectoryReader> intensityReader(optionIntensities.as<std::string>());
			pipeline::Process<ImageStackDirectoryReader> labelReader(optionLabels.as<std::string>());

			intensityStack = intensityReader->getOutput();
			labelStack = labelReader->getOutput();

			intensities = ExplicitVolume<float>(*intensityStack);
			labels      = ExplicitVolume<int>(*labelStack);
		}

		if (optionExtractLabels) {

			LOG_DEBUG(logger::out) << "[main] extracting labels from connected components" << std::endl;

			pipeline::Process<ExtractLabels> extractLabels;
			extractLabels->setInput(labelStack);
			labelStack = extractLabels->getOutput();
		}

		unsigned int width  = labels.width();
		unsigned int height = labels.height();
		unsigned int depth  = labels.depth();

		if (width  != intensities.width() ||
			height != intensities.height() ||
			depth  != intensities.depth())
			UTIL_THROW_EXCEPTION(
					UsageError,
					"intensity and label volumes have different sizes");

		// store them in the project file

		boost::filesystem::remove(optionProjectFile.as<std::string>());
		Hdf5VolumeStore volumeStore(optionProjectFile.as<std::string>());

		volumeStore.saveIntensities(intensities);
		volumeStore.saveLabels(labels);

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
	}
}
