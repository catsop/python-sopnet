/**
 * This program reads a volume that contains labels for tubes (i.e., pixels that 
 * are supposed to belong to the same neural process have the same label) from 
 * an HDF5 file, and creates a sequence of RGBA images that can be imported into 
 * raveler (https://openwiki.janelia.org/wiki/display/flyem/Raveler) 
 */

#include <iostream>
#include <fstream>

#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/exceptions.h>
#include <util/assert.h>
#include <volumes/io/Hdf5VolumeStore.h>
#include <vigra/impex.hxx>
#include <vigra/slic.hxx>
#include <vigra/multi_watersheds.hxx>

util::ProgramOption optionProjectFile(
		util::_long_name        = "projectFile",
		util::_short_name       = "p",
		util::_description_text = "The project file to read the label volume.",
		util::_default_value    = "project.hdf");

util::ProgramOption optionOutputDirectory(
		util::_long_name        = "outputDirectory",
		util::_short_name       = "o",
		util::_description_text = "A directory to store the raveler label files.",
		util::_default_value    = "labels");

util::ProgramOption optionSlicSuperpixels(
		util::_long_name        = "slicSuperpixels",
		util::_description_text = "Use SLIC superpixels instead of watersheds to obtain raveler superpixels.");

util::ProgramOption optionSlicIntensityScaling(
		util::_long_name        = "slicIntensityScaling",
		util::_description_text = "How to scale the image intensity for comparison to spatial distance. Default is 1.0.",
		util::_default_value    = 1.0);

util::ProgramOption optionSliceSize(
		util::_long_name        = "slicSize",
		util::_description_text = "An upper limit on the SLIC superpixel size. Default is 10.",
		util::_default_value    = 10);

int main(int argc, char** argv) {

	try {

		typedef int Label;
		typedef int SpId;
		typedef int SegmentId;
		typedef int BodyId;

		util::ProgramOptions::init(argc, argv);
		logger::LogManager::init();

		// read the label and membrane volume

		ExplicitVolume<Label> labels;
		ExplicitVolume<float> membranes;

		LOG_USER(logger::out) << "reading labels and membranes..." << std::endl;

		Hdf5VolumeStore volumeStore(optionProjectFile.as<std::string>());
		volumeStore.retrieveLabels(labels);
		volumeStore.retrieveMembranes(membranes);

		// map from reconstruction label to segment ids
		std::map<Label, std::vector<SegmentId>> bodies;

		// superpixel id image
		vigra::MultiArray<2, float> section(vigra::Shape2(labels.width(), labels.height()));
		std::string outDir = optionOutputDirectory;

		int nextSegmentId = 1;

		// for each section...
		for (unsigned int z = 0; z < labels.depth(); z++) {

			LOG_USER(logger::out) << "processing section " << z << "..." << std::endl;

			// extract superpixels
			vigra::MultiArray<2, int> superpixels(labels.width(), labels.height());

			if (optionSlicSuperpixels) {

				vigra::slicSuperpixels(
						membranes.data().bind<2>(z),
						superpixels,
						optionSlicIntensityScaling.as<double>(),
						optionSliceSize.as<double>(),
						vigra::SlicOptions().iterations(100));

			} else {

				vigra::watershedsMultiArray(
						membranes.data().bind<2>(z),
						superpixels,
						vigra::IndirectNeighborhood,
						vigra::WatershedOptions().seedOptions(vigra::SeedOptions().extendedMinima()));
			}

			// find superpixel overlaps with labels

			std::map<SpId, std::map<Label, int>> superpixelOverlaps;

			for (unsigned int y = 0; y < labels.height(); y++)
			for (unsigned int x = 0; x < labels.width(); x++) {

				Label label = labels(x, y, z);
				SpId  sp    = superpixels(x, y);

				superpixelOverlaps[sp][label]++;
			}

			// match superpixels to reconstruction label

			std::map<SpId, Label> spToLabel;

			for (const auto& spOverlap : superpixelOverlaps) {

				SpId  spId       = spOverlap.first;
				int   maxOverlap = 0;
				Label bestLabel  = 0;

				for (const auto& labelOverlap : spOverlap.second) {

					Label label   = labelOverlap.first;
					int   overlap = labelOverlap.second;

					if (overlap > maxOverlap) {

						maxOverlap = overlap;
						bestLabel  = label;
					}
				}

				spToLabel[spId] = bestLabel;
			}

			// draw all superpixels with their best label color
			for (unsigned int y = 0; y < labels.height(); y++)
			for (unsigned int x = 0; x < labels.width(); x++) {

				SpId sp = superpixels(x, y);
				section(x, y) = spToLabel[sp];
			}

			std::stringstream filename;
			filename << outDir << "/labels" << std::setw(8) << std::setfill('0') << z << ".tif";

			vigra::exportImage(
					section,
					vigra::ImageExportInfo(filename.str().c_str()));
		}

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
	}
}

