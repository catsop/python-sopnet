/**
 * This program produces a label image stack (with labels as the assembly hash)
 * from a membrane probability image stack, a raw image stack and a set of
 * feature weights. This is intended as a minimal demonstration of segmentation
 * without requiring a persistence provider, so local stores are used. This
 * requires that all extracted data fit in memory. Slice and segment extraction
 * can be performed blockwise, but solution is limited to a single core to
 * produce univocal labels.
 */
#include <string>
#include <fstream>
#include <iostream>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem.hpp>

#include <vigra/multi_impex.hxx>

#include <util/exceptions.h>
#include <util/Logger.h>
#include <util/point.hpp>
#include <util/ProgramOptions.h>
#include <pipeline/all.h>
#include <pipeline/Value.h>
#include <pipeline/Process.h>
#include <segments/Segments.h>
#include <imageprocessing/ExplicitVolume.h>
#include <imageprocessing/ImageStack.h>
#include <imageprocessing/io/ImageStackDirectoryReader.h>
#include <blockwise/guarantors/SegmentGuarantor.h>
#include <blockwise/guarantors/SliceGuarantor.h>
#include <blockwise/guarantors/SolutionGuarantor.h>
#include <blockwise/blocks/Block.h>
#include <blockwise/blocks/Blocks.h>
#include <blockwise/persistence/SegmentDescriptions.h>
#include <blockwise/persistence/local/LocalSegmentStore.h>
#include <blockwise/persistence/local/LocalSliceStore.h>
#include <blockwise/persistence/local/LocalStackStore.h>
#include <features/SegmentFeaturesExtractor.h>


util::ProgramOption optionMembranesPath(
	util::_long_name = 			"membranes",
	util::_description_text = 	"Path to membrane image stack",
	util::_default_value =		"./membranes");

util::ProgramOption optionRawImagesPath(
	util::_long_name = 			"raw",
	util::_description_text = 	"Path to raw image stack",
	util::_default_value =		"./raw");

util::ProgramOption optionLabelImagesPath(
	util::_long_name = 			"labels",
	util::_description_text = 	"Path to output segmentated label images",
	util::_default_value =		"./labels");

util::ProgramOption optionBlockSizeFraction(
	util::_long_name = 			"blockSizeFraction",
	util::_description_text = 	"Block size, as a fraction of the whole stack, like numerator/denominator",
	util::_default_value =		"2/5");

util::ProgramOption optionForceExplanation(
	util::_long_name = 			"forceExplanation",
	util::_description_text = 	"Force explanation of all conflict cliques",
	util::_default_value =		true);

util::ProgramOption optionFeatureWeightsFile(
	util::_long_name = 			"featureWeightsFile",
	util::_description_text = 	"Feature weights file (one weight per line)",
	util::_default_value =		"feature_weights.dat");

util::ProgramOption optionCorePadding(
	util::_long_name = 			"padding",
	util::_description_text = 	"Core padding in blocks (ineffectual for single-core)",
	util::_default_value =		"2");



bool guaranteeSegments(
		const ProjectConfiguration& configuration,
		const boost::shared_ptr<SliceStore> sliceStore,
		const boost::shared_ptr<SegmentStore> segmentStore,
		const boost::shared_ptr<StackStore> membraneStackStore,
		const boost::shared_ptr<StackStore> rawStackStore) {

	BlockUtils blockUtils(configuration);
	Blocks blocks = blockUtils.getBlocksInBox(blockUtils.getVolumeBoundingBox());
	SliceGuarantor sliceGuarantor(configuration, sliceStore, membraneStackStore);
	SegmentGuarantor segmentGuarantor(configuration, segmentStore, sliceStore, rawStackStore);

	LOG_DEBUG(logger::out) << "Begin extracting segments" << std::endl;

	for (const Block& block : blocks) {

		Blocks missingBlocks;
		Blocks singleBlock;
		singleBlock.add(block);

		LOG_DEBUG(logger::out) << "Extracting slices from block " << block << std::endl;

		do {

			missingBlocks = segmentGuarantor.guaranteeSegments(singleBlock);
			if (!missingBlocks.empty()) {

				sliceGuarantor.guaranteeSlices(missingBlocks);
			}
		} while (!missingBlocks.empty());
	}

	LOG_DEBUG(logger::out) << "Done extracting segments" << std::endl;

	return true;
}


bool coreSolver(
		const ProjectConfiguration& configuration,
		const boost::shared_ptr<SliceStore> sliceStore,
		const boost::shared_ptr<SegmentStore> segmentStore,
		const boost::shared_ptr<StackStore> membraneStackStore,
		const boost::shared_ptr<StackStore> rawStackStore) {

	// Block pipeline variables
	bool bfe = optionForceExplanation;
	unsigned int padding = optionCorePadding.as<unsigned int>();

	Blocks missingBlocks;
	BlockUtils blockUtils(configuration);
	Core core = blockUtils.getCoreAtLocation(util::point<unsigned int, 3>(0, 0, 0));

	SolutionGuarantor solutionGuarantor(
			configuration,
			segmentStore,
			sliceStore,
			padding,
			bfe,
			false,
			false);

	do {

		LOG_DEBUG(logger::out) << "Attempting to guarantee a solution" << std::endl;

		missingBlocks = solutionGuarantor.guaranteeSolution(core);

		LOG_DEBUG(logger::out) << "Need segments for " << missingBlocks.size() << " blocks" << std::endl;

		if (!missingBlocks.empty() &&
			!guaranteeSegments(configuration, sliceStore, segmentStore,
							   membraneStackStore, rawStackStore)) {

			return false;
		}
	} while (!missingBlocks.empty());

	return true;
}


boost::shared_ptr<Slices> getSlicesBySegmentHashes(
		const boost::shared_ptr<Slices> allSlices,
		const boost::shared_ptr<SegmentDescriptions> segments,
		const std::set<SegmentHash>& segmentHashes) {

	boost::shared_ptr<Slices> slices = boost::make_shared<Slices>();

	std::set<SliceHash> sliceHashes;

	for (const SegmentDescription& segment : *segments) {

		if (segmentHashes.count(segment.getHash())) {

			const std::vector<SliceHash>& segmentLeftSlices = segment.getLeftSlices();
			sliceHashes.insert(segmentLeftSlices.begin(), segmentLeftSlices.end());
			const std::vector<SliceHash>& segmentRightSlices = segment.getRightSlices();
			sliceHashes.insert(segmentRightSlices.begin(), segmentRightSlices.end());
		}
	}

	for (boost::shared_ptr<Slice> slice : *allSlices) {

		if (sliceHashes.count(slice->hashValue())) {

			slices->add(slice);
		}
	}

	return slices;
}


ExplicitVolume<SegmentHash> writeLabels(
		const ProjectConfiguration& configuration,
		const boost::shared_ptr<SliceStore> sliceStore,
		const boost::shared_ptr<SegmentStore> segmentStore) {

	BlockUtils blockUtils(configuration);
	Core core = blockUtils.getCoreAtLocation(util::point<unsigned int, 3>(0, 0, 0));
	Cores cores;
	cores.add(core);

	std::vector<std::set<SegmentHash>> assemblies = segmentStore->getSolutionByCores(cores);

	// Generate assembly hashes.
	std::vector<SegmentHash> assemblyHashes(assemblies.size());
	for (unsigned int i = 0; i < assemblies.size(); i++) {
		const std::set<SegmentHash> segmentHashes = assemblies[i];
		SegmentHash assemblyHash = 0;

		for (const SegmentHash& segmentHash : segmentHashes) {
			boost::hash_combine(assemblyHash, segmentHash);
		}

		assemblyHashes[i] = assemblyHash;
	}

	const util::point<unsigned int, 3>& stackSize = configuration.getVolumeSize();
	ExplicitVolume<SegmentHash> labels(stackSize.x(), stackSize.y(), stackSize.z());
	labels.data() = 0;

	Blocks coreBlocks = blockUtils.getCoreBlocks(core);

	for (const Block& block : coreBlocks) {

		Blocks blocks;
		blocks.add(block);
		Blocks missingBlocks;
		boost::shared_ptr<SegmentDescriptions> segments = segmentStore->getSegmentsByBlocks(
				blocks,
				missingBlocks,
				false);

		boost::shared_ptr<Slices> allSlices = sliceStore->getSlicesByBlocks(blocks, missingBlocks);

		if (!missingBlocks.empty()) {
			UTIL_THROW_EXCEPTION(
					Exception,
					"Attempt to retrieve segments or slices for missing blocks.");
		}

		#pragma omp parallel for
		for (unsigned int i = 0; i < assemblies.size(); i++) {

			boost::shared_ptr<Slices> slices = getSlicesBySegmentHashes(allSlices, segments, assemblies[i]);

			for (boost::shared_ptr<Slice> slice : *slices) {

				int z = slice->getSection();
				for (const util::point<unsigned int, 2>& p : slice->getComponent()->getPixels())
					labels(p.x(), p.y(), z) = assemblyHashes[i];
			}
		}
	}

	return labels;
}


unsigned int fractionCeiling(unsigned int m, unsigned int num, unsigned int denom) {

	return (m * num + denom - 1) / denom;
}

util::point<unsigned int, 3> parseBlockSize(
		std::string blockSizeFraction,
		const util::point<unsigned int, 3> stackSize) {

	std::string item;
	util::point<unsigned int, 3> blockSize;
	int num, denom;

	std::size_t slashPos = blockSizeFraction.find("/");

	if (slashPos == std::string::npos) {

		LOG_DEBUG(logger::out) << "Got no fraction in block size parameter, setting to stack size" <<
			std::endl;
		blockSize = stackSize;
	} else {

		std::string numStr = blockSizeFraction.substr(0, slashPos);
		std::string denomStr = blockSizeFraction.substr(slashPos + 1, std::string::npos);

		LOG_DEBUG(logger::out) << "Got numerator " << numStr << ", denominator " << denomStr << std::endl;
		num = boost::lexical_cast<int>(numStr);
		denom = boost::lexical_cast<int>(denomStr);

		blockSize = util::point<unsigned int, 3>(
				fractionCeiling(stackSize.x(), num, denom),
				fractionCeiling(stackSize.y(), num, denom),
				stackSize.z() / 2 + 1);
	}

	LOG_DEBUG(logger::out) << "Stack size: " << stackSize << ", block size: " << blockSize << std::endl;

	return blockSize;
}


void mkdir(const std::string& path) {

	boost::filesystem::path dir(path);
	boost::filesystem::create_directory(dir);
}

int main(int optionc, char** optionv) {

	util::ProgramOptions::init(optionc, optionv);
	logger::LogManager::init();

	try {

		std::string membranePath = optionMembranesPath.as<std::string>();
		pipeline::Value<ImageStack> testStack;
		unsigned int nx, ny, nz;
		util::point<unsigned int, 3> stackSize, blockSize, coreSize;

		boost::shared_ptr<ImageStackDirectoryReader> directoryStackReader =
			boost::make_shared<ImageStackDirectoryReader>(membranePath);

		testStack = directoryStackReader->getOutput();
		nx = testStack->width();
		ny = testStack->height();
		nz = testStack->size();

		stackSize = util::point<unsigned int, 3>(nx, ny, nz);

		std::string blockSizeFraction = optionBlockSizeFraction.as<std::string>();
		blockSize = parseBlockSize(blockSizeFraction, stackSize);

		// Only one core, so core size is equal to the number of blocks.
		coreSize = (stackSize + blockSize - util::point<unsigned int, 3>(1, 1, 1))/blockSize;

		std::ifstream weightsFile(optionFeatureWeightsFile.as<std::string>().c_str());
		if (!weightsFile)
			UTIL_THROW_EXCEPTION(IOError, "Failed to open feature weights file.");
		std::vector<double> featureWeights;
		double weight;
		while (weightsFile >> weight)
			featureWeights.push_back(weight);

		ProjectConfiguration pc;
		pc.setBackendType(ProjectConfiguration::Local);
		pc.setVolumeSize(stackSize);
		pc.setBlockSize(blockSize);
		pc.setCoreSize(coreSize);
		pc.setLocalFeatureWeights(featureWeights);

		LOG_USER(logger::out) << "Creating output directories" << std::endl;
		mkdir(optionLabelImagesPath.as<std::string>());

		testStack->clear();

		// Create stores.
		std::string rawPath = optionRawImagesPath.as<std::string>();
		boost::shared_ptr<StackStore> membraneStackStore = boost::make_shared<LocalStackStore>(membranePath);
		boost::shared_ptr<StackStore> rawStackStore = boost::make_shared<LocalStackStore>(rawPath);

		boost::shared_ptr<SliceStore> sliceStore = boost::make_shared<LocalSliceStore>();
		boost::shared_ptr<SegmentStore> segmentStore = boost::make_shared<LocalSegmentStore>(pc);

		if (!coreSolver(pc, sliceStore, segmentStore, membraneStackStore, rawStackStore))
			return -1;

		ExplicitVolume<SegmentHash> labels = writeLabels(pc, sliceStore, segmentStore);

		std::ostringstream baseFileName;
		baseFileName << optionLabelImagesPath.as<std::string>() << "/labels";
		vigra::VolumeExportInfo info(baseFileName.str().c_str(), ".png");
		vigra::exportVolume(labels.data(), info);

		return 0;
	} catch (boost::exception& e) {

		handleException(e, std::cerr);
	}
}
