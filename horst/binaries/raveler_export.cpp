/**
 * This program reads a volume that contains labels for tubes (i.e., pixels that 
 * are supposed to belong to the same neural process have the same label) from 
 * an HDF5 file, and creates a sequence of RGBA images that can be imported into 
 * raveler (https://openwiki.janelia.org/wiki/display/flyem/Raveler) 
 */

#include <iostream>

#include <vigra/impex.hxx>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/exceptions.h>
#include <volumes/io/Hdf5VolumeStore.h>

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

int main(int argc, char** argv) {

	try {

		util::ProgramOptions::init(argc, argv);
		logger::LogManager::init();

		// read the label volume

		ExplicitVolume<int> labels;

		Hdf5VolumeStore volumeStore(optionProjectFile.as<std::string>());
		volumeStore.retrieveLabels(labels);

		// convert z-slices into RGBA images

		vigra::MultiArray<2, vigra::RGBValue<unsigned char>> slice(vigra::Shape2(labels.width(), labels.height()));

		std::string outDir = optionOutputDirectory;

		for (unsigned int z = 0; z < labels.depth(); z++) {

			for (unsigned int y = 0; y < labels.height(); y++)
			for (unsigned int x = 0; x < labels.width(); x++) {

				int id = labels(x, y, z);
				slice(x, y) = vigra::RGBValue<unsigned char>(
						id,
						id >> 8,
						id >> 16);
			}

			std::stringstream filename;
			filename << outDir << "/labels" << std::setw(8) << std::setfill('0') << z << ".png";

			vigra::exportImage(
					slice,
					vigra::ImageExportInfo(filename.str().c_str()));
		}

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
	}
}

