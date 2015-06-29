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
		vigra::MultiArray<2, vigra::RGBValue<unsigned char>> section(vigra::Shape2(labels.width(), labels.height()));
		std::string outDir = optionOutputDirectory;

		// output files
		std::ofstream segmentfile(optionOutputDirectory.as<std::string>() + "/superpixel_to_segment_map.txt");
		std::ofstream bodyfile(optionOutputDirectory.as<std::string>() + "/segment_to_body_map.txt");

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

			// find superpixel overlaps with labels, and create raveler id map 
			// on the fly

			std::map<SpId, std::map<Label, int>> superpixelOverlaps;

			for (unsigned int y = 0; y < labels.height(); y++)
			for (unsigned int x = 0; x < labels.width(); x++) {

				Label label = labels(x, y, z);
				SpId  sp    = superpixels(x, y);

				if (x == 1276 && y == 1602)
					std::cout << z << ": label is " << label << std::endl;

				superpixelOverlaps[sp][label]++;

				// superpixel ids are supposed to be stored in RGB as 24 bit 
				// integers: most significant bits in R, least in B
				section(x, y) = vigra::RGBValue<unsigned char>(
						/* R */ sp,
						/* G */ sp >> 8,
						/* B */ sp >> 16);

				SpId test =
						section(x, y)[0] +
						section(x, y)[1]*256 +
						section(x, y)[2]*256*256;
				UTIL_ASSERT_REL(test, ==, sp);
			}

			// hysterical raisins
			section(0, 0) = 0;

			// match superpixels to reconstruction label -> segments

			std::map<Label, std::set<SpId>> segments;

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

				segments[bestLabel].insert(spId);
			}

			// draw all superpixels that overlap mostly with background as 0
			for (unsigned int y = 0; y < labels.height(); y++)
			for (unsigned int x = 0; x < labels.width(); x++) {

				SpId sp = superpixels(x, y);

				if (segments[0].count(sp))
					section(x, y) = 0;
			}

			std::stringstream filename;
			filename << outDir << "/labels" << std::setw(8) << std::setfill('0') << z << ".png";

			vigra::exportImage(
					section,
					vigra::ImageExportInfo(filename.str().c_str()));

			// translate matches' labels to volume unique segment ids

			// hysterical raisins
			segmentfile << z << "\t" << 0 << "\t" << 0 << std::endl;;

			for (const auto& segment : segments) {

				Label label = segment.first;

				if (label == 0)
					continue;

				SegmentId segmentId = nextSegmentId++;

				for (SpId spId : segment.second)
					segmentfile << z << "\t" << spId << "\t" << segmentId << std::endl;;

				if (label != 0)
					bodies[label].push_back(segmentId);
			}
		}

		// store body associations

		// hysterical raisins
		bodyfile << 0 << "\t" << 0 << std::endl;

		for (const auto& body : bodies) {

			// body id = label id
			BodyId bodyId = body.first;

			for (SegmentId segmentId : body.second)
				bodyfile << segmentId << "\t" << bodyId << std::endl;
		}

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
	}
}

