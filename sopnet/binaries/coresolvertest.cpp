#include <string>
#include <fstream>
#include <iostream>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem.hpp>

#include <vigra/impex.hxx>

#include <util/exceptions.h>
#include <util/Logger.h>
#include <util/point.hpp>
#include <util/ProgramOptions.h>
#include <pipeline/all.h>
#include <pipeline/Value.h>
#include <pipeline/Process.h>
#include <inference/PriorCostFunctionParameters.h>
#include <inference/SegmentationCostFunctionParameters.h>
#include <slices/ComponentTreeConverter.h>
#include <slices/SliceExtractor.h>
#include <segments/Segments.h>
#include <segments/SegmentExtractor.h>
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


util::ProgramOption optionCoreTestMembranesPath(
	util::_module = 			"core",
	util::_long_name = 			"membranes",
	util::_description_text = 	"Path to membrane image stack",
	util::_default_value =		"./membranes");

util::ProgramOption optionCoreTestRawImagesPath(
	util::_module = 			"core",
	util::_long_name = 			"raw",
	util::_description_text = 	"Path to raw image stack",
	util::_default_value =		"./raw");

util::ProgramOption optionCoreTestBlockSizeFraction(
	util::_module = 			"core",
	util::_long_name = 			"blockSizeFraction",
	util::_description_text = 	"Block size, as a fraction of the whole stack, like numerator/denominator",
	util::_default_value =		"2/5");

util::ProgramOption optionCoreTestCoreSizeFraction(
	util::_module = 			"core",
	util::_long_name = 			"coreSizeFraction",
	util::_description_text = 	"Core size, as a fraction of the whole stack, like numerator/denominator",
	util::_default_value =		"1/1");

util::ProgramOption optionCoreTestSequential(
	util::_module = 			"core",
	util::_long_name = 			"sequential",
	util::_description_text = 	"Determines whether to extract slices/segments sequentially or simultaneously",
	util::_default_value =		true);

util::ProgramOption optionCoreTestForceExplanation(
	util::_module = 			"core",
	util::_long_name = 			"forceExplanation",
	util::_description_text = 	"Force Explanation",
	util::_default_value =		true);

util::ProgramOption optionCoreTestFeatureWeightsFile(
	util::_module = 			"core",
	util::_long_name = 			"featureWeightsFile",
	util::_description_text = 	"Feature weights file (one weight per line)",
	util::_default_value =		"feature_weights.dat");

util::ProgramOption optionCoreTestWriteSliceImages(
	util::_module = 			"core",
	util::_long_name = 			"writeSliceImages",
	util::_description_text = 	"Write slice images");

util::ProgramOption optionCoreTestWriteDebugFiles(
	util::_module = 			"core",
	util::_long_name = 			"writeDebugFiles",
	util::_description_text = 	"Write debug files");

util::ProgramOption optionCoreTestDisableSliceTest(
	util::_module = 			"core",
	util::_long_name = 			"disableSlice",
	util::_description_text = 	"disable slice test");

util::ProgramOption optionCoreTestDisableSegmentTest(
	util::_module = 			"core",
	util::_long_name = 			"disableSegment",
	util::_description_text = 	"disable segment test");

util::ProgramOption optionCoreTestDisableSolutionTest(
	util::_module = 			"core",
	util::_long_name = 			"disableSolution",
	util::_description_text = 	"disable solution test");

util::ProgramOption optionCorePadding(
	util::_module = 			"core",
	util::_long_name = 			"padding",
	util::_description_text = 	"core padding in blocks",
	util::_default_value =		"2");

std::string sopnetOutputPath = "./out-sopnet";
std::string blockwiseOutputPath = "./out-blockwise";


void mkdir(const std::string& path)
{
	boost::filesystem::path dir(path);
	boost::filesystem::create_directory(dir);
}


void writeConflictSets(const boost::shared_ptr<ConflictSets> conflictSets,
					   const std::string& path)
{
	std::ofstream conflictFile;
	std::string conflictFilePath = path + "/conflict.txt";
	conflictFile.open(conflictFilePath.c_str());

	for (const ConflictSet conflictSet : *conflictSets)
	{
		for (unsigned int id : conflictSet.getSlices())
		{
			conflictFile << id << " ";
		}
		conflictFile << std::endl;
	}

	conflictFile.close();

}

void writeSlice(const Slice& slice, const std::string& sliceImageDirectory, std::ofstream& file) {

	unsigned int section = slice.getSection();
	unsigned int id      = slice.getId();
	util::box<int, 2> bbox = slice.getComponent()->getBoundingBox();

	std::string filename = sliceImageDirectory + "/" + boost::lexical_cast<std::string>(section) +
		"/" + boost::lexical_cast<std::string>(id) + "_" + boost::lexical_cast<std::string>(bbox.min().x()) +
		"_" + boost::lexical_cast<std::string>(bbox.min().y()) + ".png";
	mkdir(sliceImageDirectory + "/" + boost::lexical_cast<std::string>(section));

	if (optionCoreTestWriteDebugFiles)
	{
		file << id << ", " << slice.hashValue() << ";" << std::endl;
	}

	if (optionCoreTestWriteSliceImages)
	{
		vigra::exportImage(vigra::srcImageRange(slice.getComponent()->getBitmap()),
						vigra::ImageExportInfo(filename.c_str()));
	}
}

void writeSlices(const boost::shared_ptr<Slices> slices,
					  const std::string path)
{
	boost::filesystem::path dir(path);

	std::ofstream sliceFile;
	std::string slicelog = path + "/sliceids.txt";

	if (!optionCoreTestWriteDebugFiles && !optionCoreTestWriteSliceImages)
	{
		return;
	}

	if (optionCoreTestWriteDebugFiles)
	{
		sliceFile.open(slicelog.c_str());
	}


	for (boost::shared_ptr<Slice> slice : *slices)
	{
		writeSlice(*slice, path, sliceFile);
	}

	if (optionCoreTestWriteDebugFiles)
	{
		sliceFile.close();
	}
}

void writeSegment(std::ofstream& file, const SegmentDescription& segment)
{
	switch (segment.getType())
	{
		case EndSegmentType:
			file << "0 ";
			break;
		case ContinuationSegmentType:
			file << "1 ";
			break;
		case BranchSegmentType:
			file << "2 ";
			break;
		default:
			file << "-1 ";
	}

	file << segment.getHash() << " ";

	for (SliceHash sliceHash : segment.getLeftSlices())
		file << 'L' << sliceHash << " ";

	for (SliceHash sliceHash : segment.getRightSlices())
		file << 'R' << sliceHash << " ";

	file << std::endl;
}

void writeSegments(const SegmentDescriptions& segments,
				   const std::string path)
{
	boost::filesystem::path dir(path);

	std::ofstream segmentFile;
	std::string segmentlog = path + "/segments.txt";

	segmentFile.open(segmentlog.c_str());

	for (const SegmentDescription& segment : segments)
	{
		writeSegment(segmentFile, segment);
	}

}

boost::shared_ptr<ConflictSets> mapConflictSets(const boost::shared_ptr<ConflictSets> conflictSets,
												std::map<unsigned int, unsigned int>& idMap)
{
	boost::shared_ptr<ConflictSets> mappedSets = boost::make_shared<ConflictSets>();
	for (ConflictSet conflictSet : *conflictSets)
	{
		ConflictSet mappedSet;

		for (unsigned int id : conflictSet.getSlices())
		{
			mappedSet.addSlice(idMap[id]);
		}

		mappedSets->add(mappedSet);
	}

	return mappedSets;
}

bool conflictSetContains(const boost::shared_ptr<ConflictSets> conflictSets,
						 const ConflictSet conflictSet)
{
	for (ConflictSet otherConflictSet : *conflictSets)
	{
		if (otherConflictSet == conflictSet)
		{
			return true;
		}
	}

	return false;
}

bool segmentsContains(const boost::shared_ptr<Segments> segments,
					  const boost::shared_ptr<Segment> segment)
{
	for (boost::shared_ptr<Segment> otherSegment : segments->getSegments())
	{
		if (*otherSegment == *segment)
		{
			return true;
		}
	}

	return false;
}

bool testSlices(const ProjectConfiguration& configuration)
{
	// SOPNET variables
	int i = 0;
	std::string membranePath = optionCoreTestMembranesPath.as<std::string>();

	boost::shared_ptr<ImageStackDirectoryReader<IntensityImage> > directoryStackReader =
			boost::make_shared<ImageStackDirectoryReader<IntensityImage> >(membranePath);
	pipeline::Value<ImageStack<IntensityImage> > stack = directoryStackReader->getOutput();

	pipeline::Value<Slices> sopnetSlices = pipeline::Value<Slices>();
	pipeline::Value<ConflictSets> sopnetConflictSets = pipeline::Value<ConflictSets>();

	LOG_DEBUG(logger::out) << "Read " << stack->size() << " images" << std::endl;

	// Extract Slices as in the original SOPNET pipeline.
	for (boost::shared_ptr<IntensityImage> image : *stack)
	{
		boost::shared_ptr<SliceExtractor<unsigned char> > extractor =
			boost::make_shared<SliceExtractor<unsigned char> >(i++, true);
		pipeline::Value<Slices> slices;
		pipeline::Value<ConflictSets> conflictSets;
		extractor->setInput("membrane", image);
		slices = extractor->getOutput("slices");
		conflictSets = extractor->getOutput("conflict sets");

		sopnetSlices->addAll(*slices);
		sopnetConflictSets->addAll(*conflictSets);
		LOG_DEBUG(logger::out) << "Read " << slices->size() << " slices" << std::endl;
	}

	LOG_DEBUG(logger::out) << "Read " << sopnetSlices->size() << " slices altogether" << std::endl;

	// Blockwise variables
	boost::shared_ptr<StackStore<IntensityImage> > membraneStackStore =
			boost::make_shared<LocalStackStore<IntensityImage> >(membranePath);
	boost::shared_ptr<SliceStore> sliceStore = boost::make_shared<LocalSliceStore>();

	SliceGuarantor sliceGuarantor(configuration, sliceStore, membraneStackStore);

	BlockUtils blockUtils(configuration);
	Blocks blocks = blockUtils.getBlocksInBox(blockUtils.getVolumeBoundingBox());

	// Now, do it blockwise
	if (optionCoreTestSequential)
	{
		for (const Block& block : blocks)
		{
			Blocks missingBlocks;
			Blocks singleBlock;
			singleBlock.add(block);

			LOG_DEBUG(logger::out) << "Extracting slices from block " << block << std::endl;

			missingBlocks = sliceGuarantor.guaranteeSlices(singleBlock);

			if (!missingBlocks.empty())
			{
				LOG_DEBUG(logger::out) << "SliceGuarantor needs images for block " <<
					block << std::endl;
				return false;
			}
		}
	}
	else
	{
		Blocks missingBlocks;

		LOG_DEBUG(logger::out) << "Extracting slices from entire stack" << std::endl;

		missingBlocks = sliceGuarantor.guaranteeSlices(blocks);

		if (!missingBlocks.empty())
		{
			LOG_DEBUG(logger::out) << "SliceGuarantor needs images for " <<
				missingBlocks.size() << " blocks:" << std::endl;
			for (const Block& block : missingBlocks)
			{
				LOG_DEBUG(logger::out) << "\t" << block << std::endl;
			}
			return false;
		}
	}


	Blocks missingBlocks;
	boost::shared_ptr<Slices> blockwiseSlices = sliceStore->getSlicesByBlocks(blocks, missingBlocks);
	boost::shared_ptr<ConflictSets> blockwiseConflictSets = sliceStore->getConflictSetsByBlocks(blocks, missingBlocks);

	LOG_DEBUG(logger::out) << "Read " << blockwiseSlices->size() << " slices block-wise" << std::endl;

	// Compare outputs
	bool ok = true;

	// Slices in blockwiseSlices that are not in sopnetSlices
	Slices::slices_type bsSlicesSetDiff;
	// vice-versa
	Slices::slices_type sbSlicesSetDiff;
	ConflictSets bsConflictSetDiff;
	ConflictSets sbConflictSetDiff;

	Slices::SliceComparator sliceComparator;

	std::set_difference(
			blockwiseSlices->begin(), blockwiseSlices->end(),
			sopnetSlices->begin(), sopnetSlices->end(),
			std::inserter(bsSlicesSetDiff, bsSlicesSetDiff.begin()), sliceComparator);

	std::set_difference(
			sopnetSlices->begin(), sopnetSlices->end(),
			blockwiseSlices->begin(), blockwiseSlices->end(),
			std::inserter(sbSlicesSetDiff, sbSlicesSetDiff.begin()), sliceComparator);

	ok = ok && bsSlicesSetDiff.size() == 0 && sbSlicesSetDiff.size() == 0;

	if (bsSlicesSetDiff.size() != 0)
	{
		LOG_USER(logger::out) << bsSlicesSetDiff.size() <<
			" slices were found in the blockwise output but not sopnet: " << std::endl;
		for (boost::shared_ptr<Slice> slice : bsSlicesSetDiff)
		{
			LOG_USER(logger::out) << slice->getId() << ", " << slice->hashValue() << std::endl;
		}
	}

	if (sbSlicesSetDiff.size() != 0)
	{
		LOG_USER(logger::out) << sbSlicesSetDiff.size() <<
			" slices were found in the sopnet output but not blockwise: " << std::endl;
		for (boost::shared_ptr<Slice> slice : sbSlicesSetDiff)
		{
			LOG_USER(logger::out) << slice->getId() << ", " << slice->hashValue() << std::endl;
		}
	}

	LOG_DEBUG(logger::out) << "Comparing " << blockwiseConflictSets->size()
			<< " blockwise conflict sets with "
			<< sopnetConflictSets->size() << " sopnet conflict sets." << std::endl;

	// Naive N^2 difference between conflict sets since an ordering is not defined.
	for (ConflictSet blockwiseConflict : *blockwiseConflictSets) {
		bool found = false;
		for (ConflictSet sopnetConflict : *sopnetConflictSets) {
			if (sopnetConflict == blockwiseConflict) {
				found = true;
				break;
			}
		}

		if (!found) bsConflictSetDiff.add(blockwiseConflict);
	}

	for (ConflictSet sopnetConflict : *sopnetConflictSets) {
		bool found = false;
		for (ConflictSet blockwiseConflict : *blockwiseConflictSets) {
			if (sopnetConflict == blockwiseConflict) {
				found = true;
				break;
			}
		}

		if (!found) sbConflictSetDiff.add(sopnetConflict);
	}

	ok = ok && bsConflictSetDiff.size() == 0 && sbConflictSetDiff.size() == 0;

	if (bsConflictSetDiff.size() != 0)
	{
		LOG_USER(logger::out) << bsConflictSetDiff.size() << '/' << blockwiseConflictSets->size() <<
			" ConflictSets were found in the blockwise output but not sopnet:" << std::endl;
		for (const ConflictSet& conflictSet : bsConflictSetDiff)
		{
			LOG_USER(logger::out) << conflictSet << std::endl;
		}
	}

	if (sbConflictSetDiff.size() != 0)
	{
		LOG_USER(logger::out) << sbConflictSetDiff.size() << '/' << sopnetConflictSets->size() <<
			" ConflictSets were found in the sopnet output but not blockwise:" << std::endl;
		for (const ConflictSet& conflictSet : sbConflictSetDiff)
		{
			LOG_USER(logger::out) << conflictSet << std::endl;
		}
	}

	writeSlices(sopnetSlices, sopnetOutputPath);
	writeSlices(blockwiseSlices, blockwiseOutputPath);

	if (optionCoreTestWriteDebugFiles)
	{
		writeConflictSets(sopnetConflictSets, sopnetOutputPath);
		writeConflictSets(blockwiseConflictSets, blockwiseOutputPath);
	}


	LOG_USER(logger::out) << "Slice test " << (ok ? "passed" : "failed") << std::endl;

	return ok;
}

bool guaranteeSegments(
		const ProjectConfiguration& configuration,
		const boost::shared_ptr<SliceStore> sliceStore,
		const boost::shared_ptr<SegmentStore> segmentStore,
		const boost::shared_ptr<StackStore<IntensityImage> > membraneStackStore,
		const boost::shared_ptr<StackStore<IntensityImage> > rawStackStore)
{
	BlockUtils blockUtils(configuration);
	Blocks blocks = blockUtils.getBlocksInBox(blockUtils.getVolumeBoundingBox());
	SliceGuarantor sliceGuarantor(configuration, sliceStore, membraneStackStore);
	SegmentGuarantor segmentGuarantor(configuration, segmentStore, sliceStore, rawStackStore);

	LOG_DEBUG(logger::out) << "Begin extracting segments" << std::endl;

	// Now, do it blockwise
	if (optionCoreTestSequential)
	{
		int count = 0;

		for (const Block& block : blocks)
		{
			Blocks missingBlocks;
			Blocks singleBlock;
			singleBlock.add(block);

			LOG_DEBUG(logger::out) << "Extracting slices from block " << block << std::endl;

			do
			{
				missingBlocks = segmentGuarantor.guaranteeSegments(singleBlock);
				if (!missingBlocks.empty())
				{
					sliceGuarantor.guaranteeSlices(missingBlocks);
				}
				++count;

				// If we have run more times than there are blocks in the block manager, then there
				// is definitely something wrong.
				if (count > (int)(blocks.size() + 2))
				{
					LOG_USER(logger::out) << "SegmentGuarantor test: too many iterations" << std::endl;
					return false;
				}
			} while (!missingBlocks.empty());
		}
	}
	else
	{
		Blocks missingBlocks;

		LOG_DEBUG(logger::out) << "Extracting slices from entire stack" << std::endl;

		sliceGuarantor.guaranteeSlices(blocks);
		missingBlocks = segmentGuarantor.guaranteeSegments(blocks);

		if (!missingBlocks.empty())
		{
			LOG_DEBUG(logger::out) << "SegmentGuarantor says it needs slices for " <<
				missingBlocks.size() << ", but we extracted slices for all of them" << std::endl;
			for (const Block& block : missingBlocks)
			{
				LOG_DEBUG(logger::out) << "\t" << block << std::endl;
			}
			return false;
		}
	}

	LOG_DEBUG(logger::out) << "Done extracting segments" << std::endl;

	return true;
}

void logSegment(const SegmentDescription& segment)
{
	LOG_DEBUG(logger::out) << "Section: " << segment.getSection() << " "
			<< Segment::typeString(segment.getType()) << " ";

	// Use hash values rather than ids because we want to cross-reference later
	// hash values are consistent across equality, whereas slice ids can vary.
	for (SliceHash sliceHash : segment.getLeftSlices())
		LOG_DEBUG(logger::out) << 'L' << sliceHash << " ";
	for (SliceHash sliceHash : segment.getRightSlices())
		LOG_DEBUG(logger::out) << 'R' << sliceHash << " ";

	LOG_DEBUG(logger::out) << std::endl;
}

bool testSegments(const ProjectConfiguration& configuration)
{
	// SOPNET variables
	int i = 0;
	bool segExtraction = false, ok = true;
	std::string membranePath = optionCoreTestMembranesPath.as<std::string>();
	std::string rawPath = optionCoreTestRawImagesPath.as<std::string>();

	boost::shared_ptr<ImageStackDirectoryReader<IntensityImage> > directoryStackReader =
			boost::make_shared<ImageStackDirectoryReader<IntensityImage> >(membranePath);
	pipeline::Value<ImageStack<IntensityImage> > stack = directoryStackReader->getOutput();

	boost::shared_ptr<ImageStackDirectoryReader<IntensityImage> > rawImageStackReader =
			boost::make_shared<ImageStackDirectoryReader<IntensityImage> >(rawPath);
	boost::shared_ptr<SegmentFeaturesExtractor> featureExtractor =
		boost::make_shared<SegmentFeaturesExtractor>();

	pipeline::Value<Segments> sopnetSegments = pipeline::Value<Segments>();
	pipeline::Value<Slices> prevSlices, nextSlices;
	pipeline::Value<Features> sopnetFeatures;

	// Extract Segments as in the original SOPNET pipeline.
	for (boost::shared_ptr<IntensityImage> image : *stack)
	{
		boost::shared_ptr<SliceExtractor<unsigned char> > extractor =
			boost::make_shared<SliceExtractor<unsigned char> >(i++, true);
		extractor->setInput("membrane", image);
		nextSlices = extractor->getOutput("slices");
		pipeline::Value<ConflictSets> conflictSets = extractor->getOutput("conflict sets");

		// Set conflict sets in slices
		std::map<SliceHash, unsigned int> internalIdMap;

		for (const boost::shared_ptr<Slice> slice : *nextSlices) {
			internalIdMap[slice->hashValue()] = slice->getId();
		}

		for (const ConflictSet& conflictSet : *conflictSets) {
			std::vector<unsigned int> setInternalIds;
			setInternalIds.reserve(conflictSet.getSlices().size());

			for (const SliceHash& sliceHash : conflictSet.getSlices())
				if (internalIdMap.count(sliceHash))
					setInternalIds.push_back(internalIdMap[sliceHash]);

			nextSlices->addConflicts(setInternalIds);
		}

		if (segExtraction)
		{
			boost::shared_ptr<SegmentExtractor> segmentExtractor =
				boost::make_shared<SegmentExtractor>();
			pipeline::Value<Segments> segments;

			segmentExtractor->setInput("previous slices", prevSlices);
			segmentExtractor->setInput("next slices", nextSlices);

			segments = segmentExtractor->getOutput("segments");
			sopnetSegments->addAll(segments);
		}

		segExtraction = true;
		prevSlices = nextSlices;
	}

	// Collect Features
	LOG_DEBUG(logger::out) << "Collecting segment features for sopnet pipeline" << std::endl;
	featureExtractor->setInput("segments", sopnetSegments);
	featureExtractor->setInput("raw sections", rawImageStackReader->getOutput());
	sopnetFeatures = featureExtractor->getOutput("all features");

	LOG_DEBUG(logger::out) << "Extracted " << sopnetFeatures->size() << " sopnet features for " <<
		sopnetSegments->size() << " segments" << std::endl;

	// Blockwise variables
	boost::shared_ptr<StackStore<IntensityImage> > rawStackStore = boost::make_shared<LocalStackStore<IntensityImage> >(rawPath);
	boost::shared_ptr<StackStore<IntensityImage> > membraneStackStore = boost::make_shared<LocalStackStore<IntensityImage> >(membranePath);

	boost::shared_ptr<SliceStore> sliceStore = boost::make_shared<LocalSliceStore>();
	boost::shared_ptr<SegmentStore> segmentStore = boost::make_shared<LocalSegmentStore>(configuration);

	// Now, do it blockwise
	if (!guaranteeSegments(configuration, sliceStore, segmentStore, membraneStackStore, rawStackStore))
	{
		return false;
	}

	LOG_DEBUG(logger::out) << "Segment test: segments extracted successfully" << std::endl;

	BlockUtils blockUtils(configuration);
	Blocks missingBlocks;
	Blocks blocks = blockUtils.getBlocksInBox(blockUtils.getVolumeBoundingBox());
	boost::shared_ptr<SegmentDescriptions> blockwiseDescriptions = segmentStore->getSegmentsByBlocks(blocks, missingBlocks, false);

	LOG_DEBUG(logger::out) << "read back " << blockwiseDescriptions->size() << " blockwise segments" <<
		std::endl;

	// Now, check for differences
	// First, check for differences in the segment sets.

	// Map SOPNET Segments to SegmentDescriptions for easy comparison.
	SegmentDescriptions sopnetDescriptions;
	for (boost::shared_ptr<Segment> segment : sopnetSegments->getSegments())
	{
		SegmentDescription segmentDescription(*segment);
		segmentDescription.setFeatures(sopnetFeatures->get(segment->getId()));
		sopnetDescriptions.add(segmentDescription);
	}

	// Segments that appear in the blockwise segments, but not the sopnet segments
	SegmentDescriptions::segments_type bsSegmentSetDiff;
	// vice-versa
	SegmentDescriptions::segments_type sbSegmentSetDiff;

	LOG_DEBUG(logger::out) << "Checking for segment equality" << std::endl;

	SegmentDescriptions::SegmentDescriptionComparator segmentComparator;
	std::set_difference(
			blockwiseDescriptions->begin(), blockwiseDescriptions->end(),
			sopnetDescriptions.begin(), sopnetDescriptions.end(),
			std::inserter(bsSegmentSetDiff, bsSegmentSetDiff.begin()), segmentComparator);

	std::set_difference(
			sopnetDescriptions.begin(), sopnetDescriptions.end(),
			blockwiseDescriptions->begin(), blockwiseDescriptions->end(),
			std::inserter(sbSegmentSetDiff, sbSegmentSetDiff.begin()), segmentComparator);

	if (bsSegmentSetDiff.size() == 0 && sbSegmentSetDiff.size() == 0)
	{
		LOG_USER(logger::out) << "Segments are consistent" << std::endl;
	}

	if (bsSegmentSetDiff.size() != 0)
	{
		LOG_USER(logger::out) << bsSegmentSetDiff.size() << '/' << blockwiseDescriptions->size() <<
			" segments were found in the blockwise output but not sopnet: " << std::endl;
		int ignored = 0;
		for (const SegmentDescription& segment : bsSegmentSetDiff)
		{
			// Ignore end segments at either extent of the volume. These are
			// intentionally extracted by blockwise SOPNET.
			if ((segment.getSection() == 0 ||
				segment.getSection() == blockUtils.getVolumeBoundingBox().max().z()) &&
				segment.getType() == EndSegmentType) {
				ignored++;
			} else {
				logSegment(segment);
				ok = false;
			}
		}

		LOG_USER(logger::out) << '(' << ignored << " were ignored ends at volume bounds)" << std::endl;
	}

	if (sbSegmentSetDiff.size() != 0)
	{
		ok = false;
		LOG_USER(logger::out) << sbSegmentSetDiff.size() << '/' << sopnetDescriptions.size() <<
			" segments were found in the sopnet output but not blockwise: " << std::endl;
		for (const SegmentDescription& segment : sbSegmentSetDiff)
		{
			logSegment(segment);
		}
	}

	// For convenience create sets with only the segments (by hash) common to
	// both.
	SegmentDescriptions::segments_type bsSegmentSetInt;
	// vice-versa
	SegmentDescriptions::segments_type sbSegmentSetInt;

	std::set_intersection(
			blockwiseDescriptions->begin(), blockwiseDescriptions->end(),
			sopnetDescriptions.begin(), sopnetDescriptions.end(),
			std::inserter(bsSegmentSetInt, bsSegmentSetInt.begin()), segmentComparator);

	std::set_intersection(
			sopnetDescriptions.begin(), sopnetDescriptions.end(),
			blockwiseDescriptions->begin(), blockwiseDescriptions->end(),
			std::inserter(sbSegmentSetInt, sbSegmentSetInt.begin()), segmentComparator);

	// For those segment common to both, check for equality in Features
	LOG_DEBUG(logger::out) << "Testing features for equality for " << bsSegmentSetInt.size()
			<< " segments" << std::endl;

	for (SegmentDescriptions::iterator si = sbSegmentSetInt.begin(),
		 bi = bsSegmentSetInt.begin();
		 si != sbSegmentSetInt.end() && bi != bsSegmentSetInt.end();
		 ++si, ++bi) {

		SegmentDescription sopnetSegment = *si;
		SegmentDescription blockwiseSegment = *bi;

		if (sopnetSegment.getFeatures() != blockwiseSegment.getFeatures()) {

			LOG_USER(logger::out) << "Features are unequal for segment hashes ("
					<< sopnetSegment.getHash() << "," << blockwiseSegment.getHash() << ")" << std::endl;
			ok = false;
		}
	}


	if (optionCoreTestWriteDebugFiles)
	{
		writeSegments(sopnetDescriptions, sopnetOutputPath);
		writeSegments(*blockwiseDescriptions, blockwiseOutputPath);
	}


	LOG_USER(logger::out) << "Segment test " << (ok ? "passed" : "failed") << std::endl;

	return ok;
}


bool coreSolver(
		const ProjectConfiguration& configuration,
		const boost::shared_ptr<SliceStore> sliceStore,
		const boost::shared_ptr<SegmentStore> segmentStore,
		const boost::shared_ptr<StackStore<IntensityImage> > membraneStackStore,
		const boost::shared_ptr<StackStore<IntensityImage> > rawStackStore)
{
	unsigned int count = 0;

	// Block pipeline variables
	bool bfe = optionCoreTestForceExplanation;
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

	do
	{
		LOG_DEBUG(logger::out) << "Attempting to guarantee a solution" << std::endl;

		missingBlocks = solutionGuarantor.guaranteeSolution(core);

		LOG_DEBUG(logger::out) << "Need segments for " << missingBlocks.size() << " blocks" << std::endl;

		if (!missingBlocks.empty() &&
			!guaranteeSegments(configuration, sliceStore, segmentStore,
							   membraneStackStore, rawStackStore))
		{
			return false;
		}

		if (++count > 3)
		{
			return false;
		}
	} while (!missingBlocks.empty());

	return true;
}


bool testSolutions(const ProjectConfiguration& configuration)
{
	bool ok = true;

	LOG_DEBUG(logger::out) << "Testing solutions" << std::endl;


	std::string membranePath = optionCoreTestMembranesPath.as<std::string>();
	std::string rawPath = optionCoreTestRawImagesPath.as<std::string>();
	boost::shared_ptr<StackStore<IntensityImage> > membraneStackStore = boost::make_shared<LocalStackStore<IntensityImage> >(membranePath);
	boost::shared_ptr<StackStore<IntensityImage> > rawStackStore = boost::make_shared<LocalStackStore<IntensityImage> >(rawPath);

	boost::shared_ptr<SliceStore> sliceStore = boost::make_shared<LocalSliceStore>();
	boost::shared_ptr<SegmentStore> segmentStore = boost::make_shared<LocalSegmentStore>(configuration);

	ok &= coreSolver(configuration,
						sliceStore, segmentStore, membraneStackStore, rawStackStore);


	LOG_USER(logger::out) << "Solution test " << (ok ? "passed" : "failed") << std::endl;

	return ok;
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


int main(int optionc, char** optionv)
{
	util::ProgramOptions::init(optionc, optionv);
	logger::LogManager::init();

	try
	{
		std::string membranePath = optionCoreTestMembranesPath.as<std::string>();
		pipeline::Value<ImageStack<IntensityImage> > testStack;
		unsigned int nx, ny, nz;
		util::point<unsigned int, 3> stackSize, blockSize, numBlocks, coreSize;

		boost::shared_ptr<ImageStackDirectoryReader<IntensityImage> > directoryStackReader =
			boost::make_shared<ImageStackDirectoryReader<IntensityImage> >(membranePath);

		testStack = directoryStackReader->getOutput();
		nx = testStack->width();
		ny = testStack->height();
		nz = testStack->size();

		stackSize = util::point<unsigned int, 3>(nx, ny, nz);

		std::string blockSizeFraction = optionCoreTestBlockSizeFraction.as<std::string>();
		blockSize = parseBlockSize(blockSizeFraction, stackSize);

		std::string coreSizeFraction = optionCoreTestCoreSizeFraction.as<std::string>();
		numBlocks = (stackSize + blockSize - util::point<unsigned int, 3>(1, 1, 1))/blockSize;
		coreSize = parseBlockSize(coreSizeFraction, numBlocks);

		std::ifstream weightsFile(optionCoreTestFeatureWeightsFile.as<std::string>().c_str());
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

		if (optionCoreTestWriteSliceImages || optionCoreTestWriteDebugFiles)
		{
			LOG_USER(logger::out) << "Creating output directories" << std::endl;
			blockwiseOutputPath = blockwiseOutputPath + "_" +
				boost::lexical_cast<std::string>(stackSize.x()) + "x" +
				boost::lexical_cast<std::string>(stackSize.y()) + "_" +
				boost::lexical_cast<std::string>(blockSize.x()) + "x" +
				boost::lexical_cast<std::string>(blockSize.y());
			sopnetOutputPath = sopnetOutputPath + "_" +
				boost::lexical_cast<std::string>(stackSize.x()) + "x" +
				boost::lexical_cast<std::string>(stackSize.y());
			mkdir(sopnetOutputPath);
			mkdir(blockwiseOutputPath);
		}

		testStack->clear();

		if (!optionCoreTestDisableSliceTest &&
			!testSlices(pc))
		{

			return -1;
		}

		if (!optionCoreTestDisableSegmentTest &&
			!testSegments(pc))
		{
			return -2;
		}

		if (!optionCoreTestDisableSolutionTest &&
			!testSolutions(pc))
		{
			return -3;
		}

		return 0;
	}
	catch (boost::exception& e)
	{
		handleException(e, std::cerr);
	}
}
