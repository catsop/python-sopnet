#include <string>
#include <fstream>
#include <iostream>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem.hpp>

#include <imageprocessing/ImageStack.h>

#include <util/point3.hpp>
#include <util/ProgramOptions.h>
#include <pipeline/all.h>
#include <pipeline/Value.h>
#include <pipeline/Process.h>
#include <sopnet/Sopnet.h>
#include <sopnet/inference/PriorCostFunctionParameters.h>
#include <sopnet/inference/SegmentationCostFunctionParameters.h>
#include <sopnet/inference/Reconstructor.h>
#include <sopnet/slices/ComponentTreeConverter.h>
#include <sopnet/segments/Segments.h>
#include <sopnet/block/Block.h>
#include <sopnet/block/Blocks.h>
#include <sopnet/block/BlockManager.h>
#include <sopnet/block/Box.h>
#include <sopnet/block/LocalBlockManager.h>
#include <imageprocessing/io/ImageStackDirectoryReader.h>
#include <imageprocessing/io/ImageBlockFactory.h>
#include <imageprocessing/io/ImageBlockFileReader.h>
#include <catmaid/CoreSolver.h>
#include <catmaid/SegmentGuarantor.h>
#include <catmaid/SliceGuarantor.h>
#include <catmaid/SolutionGuarantor.h>
#include <catmaid/persistence/CostReader.h>
#include <catmaid/persistence/CostWriter.h>
#include <catmaid/persistence/LocalSegmentStore.h>
#include <catmaid/persistence/LocalSliceStore.h>
#include <catmaid/persistence/LocalStackStore.h>
#include <catmaid/persistence/SegmentFeatureReader.h>
#include <catmaid/persistence/SegmentSolutionReader.h>
#include <catmaid/persistence/SegmentReader.h>
#include <catmaid/persistence/SegmentPointerHash.h>
#include <sopnet/segments/SegmentSet.h>
#include <sopnet/features/SegmentFeaturesExtractor.h>


#include <sopnet/block/Box.h>
#include <vigra/impex.hxx>
#include <sopnet/slices/SliceExtractor.h>
#include <gui/Window.h>
#include <imageprocessing/gui/ImageStackView.h>
#include <neurons/NeuronExtractor.h>

#include <util/Logger.h>

using util::point3;
using namespace logger;
using namespace std;

point3<unsigned int> tempCoreSize = point3<unsigned int>(2, 2, 1);

//logger::LogChannel coretestlog("coretestlog", "[CoreTest] ");

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

util::ProgramOption optionCoreBuffer(
	util::_module = 			"core",
	util::_long_name = 			"buffer",
	util::_description_text = 	"core buffer in blocks",
	util::_default_value =		"2");

std::string sopnetOutputPath = "./out-sopnet";
std::string blockwiseOutputPath = "./out-blockwise";

int experiment = 0;


void handleException(boost::exception& e) {

	LOG_ERROR(out) << "caught exception: ";

	if (boost::get_error_info<error_message>(e))
		LOG_ERROR(out) << *boost::get_error_info<error_message>(e);

	if (boost::get_error_info<stack_trace>(e))
		LOG_ERROR(out) << *boost::get_error_info<stack_trace>(e);

	LOG_ERROR(out) << std::endl;

	LOG_ERROR(out) << "details: " << std::endl
	               << boost::diagnostic_information(e)
	               << std::endl;

	exit(-1);
}

void mkdir(const std::string& path)
{
	boost::filesystem::path dir(path);
	boost::filesystem::create_directory(dir);
}


void writeConflictSets(const boost::shared_ptr<ConflictSets> conflictSets,
					   const std::string& path)
{
	ofstream conflictFile;
	std::string conflictFilePath = path + "/conflict.txt";
	conflictFile.open(conflictFilePath.c_str());
	
	foreach (const ConflictSet conflictSet, *conflictSets)
	{
		foreach (unsigned int id, conflictSet.getSlices())
		{
			conflictFile << id << " ";
		}
		conflictFile << endl;
	}
	
	conflictFile.close();
	
}

void writeSlice(const Slice& slice, const std::string& sliceImageDirectory, ofstream& file) {

	unsigned int section = slice.getSection();
	unsigned int id      = slice.getId();
	util::rect<int> bbox = slice.getComponent()->getBoundingBox();

	std::string filename = sliceImageDirectory + "/" + boost::lexical_cast<std::string>(section) +
		"/" + boost::lexical_cast<std::string>(id) + "_" + boost::lexical_cast<std::string>(bbox.minX) +
		"_" + boost::lexical_cast<std::string>(bbox.minY) + ".png";
	mkdir(sliceImageDirectory + "/" + boost::lexical_cast<std::string>(section));

	if (optionCoreTestWriteDebugFiles)
	{
		file << id << ", " << slice.hashValue() << ";" << endl;
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
	
	ofstream sliceFile;
	string slicelog = path + "/sliceids.txt";

	if (!optionCoreTestWriteDebugFiles && !optionCoreTestWriteSliceImages)
	{
		return;
	}
	
	if (optionCoreTestWriteDebugFiles)
	{
		sliceFile.open(slicelog.c_str());
	}

	
	foreach (boost::shared_ptr<Slice>& slice, *slices)
	{
		writeSlice(*slice, path, sliceFile);
	}
	
	if (optionCoreTestWriteDebugFiles)
	{
		sliceFile.close();
	}
}

unsigned int fractionCeiling(unsigned int m, unsigned int num, unsigned int denom)
{
	unsigned int result = m * num / denom;
	if (result * denom < m * num)
	{	
		result += 1;
	}
	return result;
}

void writeSegment(ofstream& file, const boost::shared_ptr<Segment> segment)
{
	int i = 0;
	
	switch (segment->getType())
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
	
	file << segment->getId() << " " << segment->hashValue() << " ";
	
	foreach (boost::shared_ptr<Slice> slice, segment->getSlices())
	{
		++i;
		file << slice->hashValue() << " ";
	}
	
	for (int j = i; j < 3; ++j)
	{
		file << "0 ";
	}
	
	file << endl;
}

void writeSegments(const boost::shared_ptr<Segments> segments,
				   const std::string path)
{
	boost::filesystem::path dir(path);
	
	ofstream segmentFile;
	std::string segmentlog = path + "/segments.txt";

	segmentFile.open(segmentlog.c_str());
	
	foreach (boost::shared_ptr<Segment> segment, segments->getSegments())
	{
		writeSegment(segmentFile, segment);
	}

}

void writeSegmentTree(ofstream& file,
	const boost::shared_ptr<SegmentTree> neuron,
	unsigned int pack)
{
	unsigned int count = 0;
	
	foreach (boost::shared_ptr<Segment> segment, neuron->getSegments())
	{
		file << segment->hashValue() << " ";
		++count;
	}
	
	for (unsigned int i = count; i < pack; ++i)
	{
		file << "-1 ";
	}
	file << endl;
}

void writeSegmentTrees(const boost::shared_ptr<SegmentTrees> trees,
					   const std::string path)
{
	boost::filesystem::path dir(path);
	unsigned int maxSize = 0;
	
	ofstream neuronFile;
	std::string neuronlog = path + "/neurons.txt";

	neuronFile.open(neuronlog.c_str());
	
	foreach (boost::shared_ptr<SegmentTree> segmentTree, *trees)
	{
		if (segmentTree->size() > maxSize)
		{
			maxSize = segmentTree->size();
		}
	}
	
	foreach (boost::shared_ptr<SegmentTree> segmentTree, *trees)
	{
		writeSegmentTree(neuronFile, segmentTree, maxSize);
	}
	
	neuronFile.close();
}

int sliceContains(const boost::shared_ptr<Slices> slices,
				   const boost::shared_ptr<Slice> slice)
{
	foreach (boost::shared_ptr<Slice> otherSlice, *slices)
	{
		if (*otherSlice == *slice)
		{
			return otherSlice->getId();
		}
	}
	
	return -1;
}

boost::shared_ptr<ConflictSets> mapConflictSets(const boost::shared_ptr<ConflictSets> conflictSets,
												std::map<unsigned int, unsigned int>& idMap)
{
	boost::shared_ptr<ConflictSets> mappedSets = boost::make_shared<ConflictSets>();
	foreach (ConflictSet conflictSet, *conflictSets)
	{
		ConflictSet mappedSet;
		
		foreach (unsigned int id, conflictSet.getSlices())
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
	foreach (ConflictSet otherConflictSet, *conflictSets)
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
	foreach (boost::shared_ptr<Segment> otherSegment, segments->getSegments())
	{
		if (*otherSegment == *segment)
		{
			return true;
		}
	}
	
	return false;
}

bool testSlices(util::point3<unsigned int> stackSize, util::point3<unsigned int> blockSize)
{
	// SOPNET variables
	int i = 0;
	std::string membranePath = optionCoreTestMembranesPath.as<std::string>();
	
	boost::shared_ptr<ImageStackDirectoryReader> directoryStackReader =
			boost::make_shared<ImageStackDirectoryReader>(membranePath);
	pipeline::Value<ImageStack> stack = directoryStackReader->getOutput();
	
	pipeline::Value<Slices> sopnetSlices = pipeline::Value<Slices>();
	pipeline::Value<ConflictSets> sopnetConflictSets = pipeline::Value<ConflictSets>();
	
	LOG_USER(out) << "Read " << stack->size() << " images" << std::endl;
	
	// Extract Slices as in the original SOPNET pipeline.
	foreach (boost::shared_ptr<Image> image, *stack)
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
		LOG_USER(out) << "Read " << slices->size() << " slices" << std::endl;
	}
	
	LOG_USER(out) << "Read " << sopnetSlices->size() << " slices altogether" << std::endl;
	
	// Blockwise variables
	boost::shared_ptr<SliceGuarantor> sliceGuarantor = boost::make_shared<SliceGuarantor>();
	boost::shared_ptr<BlockManager> blockManager = boost::make_shared<LocalBlockManager>(stackSize,
																					blockSize,
																					tempCoreSize);
	boost::shared_ptr<StackStore> stackStore = boost::make_shared<LocalStackStore>(membranePath);
	boost::shared_ptr<Box<> > stackBox = boost::make_shared<Box<> >(util::point3<unsigned int>(0,0,0),
															   stackSize);
	boost::shared_ptr<Blocks> blocks = blockManager->blocksInBox(stackBox);
	boost::shared_ptr<SliceStore> sliceStore = boost::make_shared<LocalSliceStore>();
	
	boost::shared_ptr<SliceReader> sliceReader = boost::make_shared<SliceReader>();
	
	pipeline::Value<Slices> blockwiseSlices;
	pipeline::Value<ConflictSets> blockwiseConflictSets;
	
	sliceGuarantor->setInput("slice store", sliceStore);
	sliceGuarantor->setInput("stack store", stackStore);
	
	
	// Now, do it blockwise
	if (optionCoreTestSequential)
	{		
		foreach (const boost::shared_ptr<Block> block, *blocks)
		{
			pipeline::Value<Blocks> needBlocks;
			boost::shared_ptr<Blocks> singleBlock = blockManager->blocksInBox(block);
			sliceGuarantor->setInput("blocks", singleBlock);
			
			LOG_USER(out) << "Extracting slices from block " << *block << endl;
			
			needBlocks = sliceGuarantor->guaranteeSlices();
			
			if (!needBlocks->empty())
			{
				LOG_USER(out) << "SliceGuarantor needs images for block " <<
					*block << endl;
				return false;
			}
		}
	}
	else
	{
		pipeline::Value<Blocks> needBlocks;
		
		LOG_USER(out) << "Extracting slices from entire stack" << endl;
		
		sliceGuarantor->setInput("blocks", blocks);
		
		needBlocks = sliceGuarantor->guaranteeSlices();
		
		if (!needBlocks->empty())
		{
			LOG_USER(out) << "SliceGuarantor needs images for " <<
				needBlocks->length() << " blocks:" << endl;
			foreach (boost::shared_ptr<Block> block, *needBlocks)
			{
				LOG_USER(out) << "\t" << *block << endl;
			}
			return false;
		}
	}
	
	sliceReader->setInput("blocks", blocks);
	sliceReader->setInput("store", sliceStore);
	
	blockwiseSlices = sliceReader->getOutput("slices");
	blockwiseConflictSets = sliceReader->getOutput("conflict sets");
	
	LOG_USER(out) << "Read " << blockwiseSlices->size() << " slices block-wise" << endl;
	
	// Compare outputs
	bool ok = true;
	
	// Slices in blockwiseSlices that are not in sopnetSlices
	boost::shared_ptr<Slices> bsSlicesSetDiff = boost::make_shared<Slices>();
	// vice-versa
	boost::shared_ptr<Slices> sbSlicesSetDiff = boost::make_shared<Slices>();
	boost::shared_ptr<ConflictSets> bsConflictSetDiff = boost::make_shared<ConflictSets>();
	boost::shared_ptr<ConflictSets> sbConflictSetDiff = boost::make_shared<ConflictSets>();
	// Here, we will store blockwise conflict sets in which the slice ids are mapped to the
	// equivalent slide id in the sopnet slices. This allows us to compare conflict sets by their
	// == operator.
	boost::shared_ptr<ConflictSets> blockwiseConflictSetsSopnetIds;
	// Maps blockwise ids to sopnet ids.
	std::map<unsigned int, unsigned int> blockwiseSopnetIDMap;
	
	foreach (boost::shared_ptr<Slice> blockwiseSlice, *blockwiseSlices)
	{
		int id = sliceContains(sopnetSlices, blockwiseSlice);
		
		if (id < 0)
		{
			bsSlicesSetDiff->add(blockwiseSlice);
			ok = false;
		}
		else
		{
			unsigned int uid = (unsigned int)id;
			blockwiseSopnetIDMap[blockwiseSlice->getId()] = uid;
		}
	}
	
	foreach (boost::shared_ptr<Slice> sopnetSlice, *sopnetSlices)
	{
		int id = sliceContains(blockwiseSlices, sopnetSlice);
		
		if (id < 0)
		{
			sbSlicesSetDiff->add(sopnetSlice);
			ok = false;
		}
	}
	
	if (!ok)
	{
		LOG_USER(out) << bsSlicesSetDiff->size() <<
			" slices were found in the blockwise output but not sopnet: " << endl;
		foreach (boost::shared_ptr<Slice> slice, *bsSlicesSetDiff)
		{
			LOG_USER(out) << slice->getId() << ", " << slice->hashValue() << endl;
		}
		
		LOG_USER(out) << sbSlicesSetDiff->size() <<
			" slices were found in the sopnet output but not blockwise: " << endl;
		foreach (boost::shared_ptr<Slice> slice, *sbSlicesSetDiff)
		{
			LOG_USER(out) << slice->getId() << ", " << slice->hashValue() << endl;
		}
	}
	
	if (ok)
	{
		blockwiseConflictSetsSopnetIds = 
			mapConflictSets(blockwiseConflictSets, blockwiseSopnetIDMap);
		LOG_USER(out) << "Blockwise conflict sets mapped to sopnet ids" << std::endl;
		
		foreach (ConflictSet blockwiseConflict, *blockwiseConflictSetsSopnetIds)
		{
			if (!conflictSetContains(sopnetConflictSets, blockwiseConflict))
			{
				bsConflictSetDiff->add(blockwiseConflict);
				ok = false;
			}
		}
		
		foreach (ConflictSet sopnetConflict, *sopnetConflictSets)
		{
			if (!conflictSetContains(blockwiseConflictSetsSopnetIds, sopnetConflict))
			{
				
				sbConflictSetDiff->add(sopnetConflict);
				ok = false;
			}
		}
		
		if (!ok)
		{
			LOG_USER(out) << bsConflictSetDiff->size() <<
				" ConflictSets were found in the blockwise output but not sopnet:" << endl;
			foreach (ConflictSet conflictSet, *bsConflictSetDiff)
			{
				LOG_USER(out) << conflictSet << endl;
			}
			
			LOG_USER(out) << sbConflictSetDiff->size() <<
				" ConflictSets were found in the sopnet output but not blockwise:" << endl;
			foreach (ConflictSet conflictSet, *sbConflictSetDiff)
			{
				LOG_USER(out) << conflictSet << endl;
			}
		}
	}
	
	writeSlices(sopnetSlices, sopnetOutputPath);
	writeSlices(blockwiseSlices, blockwiseOutputPath);
	
	if (optionCoreTestWriteDebugFiles && blockwiseConflictSetsSopnetIds)
	{
		writeConflictSets(sopnetConflictSets, sopnetOutputPath);
		writeConflictSets(blockwiseConflictSetsSopnetIds, blockwiseOutputPath);
	}
	
	if (!ok)
	{
		LOG_USER(out) << "Slice test failed" << endl;
	}
	else
	{
		LOG_USER(out) << "Slice test passed" << endl;
	}
	
	return ok;
}

bool guaranteeSegments(const boost::shared_ptr<Blocks> blocks,
	const boost::shared_ptr<SliceStore> sliceStore,
	const boost::shared_ptr<SegmentStore> segmentStore,
	const boost::shared_ptr<StackStore> membraneStackStore,
	const boost::shared_ptr<StackStore> rawStackStore)
{
	boost::shared_ptr<SliceGuarantor> sliceGuarantor = boost::make_shared<SliceGuarantor>();
	boost::shared_ptr<SegmentGuarantor> segmentGuarantor = boost::make_shared<SegmentGuarantor>();
	boost::shared_ptr<BlockManager> blockManager = blocks->getManager();
	
	LOG_USER(out) << "Begin extracting segments" << std::endl;
	
	sliceGuarantor->setInput("slice store", sliceStore);
	sliceGuarantor->setInput("stack store", membraneStackStore);
	segmentGuarantor->setInput("segment store", segmentStore);
	segmentGuarantor->setInput("slice store", sliceStore);
	segmentGuarantor->setInput("stack store", rawStackStore);
	
	if (optionCoreTestSequential)
	{
		foreach (const boost::shared_ptr<Block> block, *blocks)
		{
			int count = 0;
			pipeline::Value<Blocks> needBlocks;
			boost::shared_ptr<Blocks> singleBlock = blockManager->blocksInBox(block);
			segmentGuarantor->setInput("blocks", singleBlock);
			
			do
			{
				needBlocks = segmentGuarantor->guaranteeSegments();
				if (!needBlocks->empty())
				{
					sliceGuarantor->setInput("blocks", needBlocks);
					sliceGuarantor->guaranteeSlices();
				}
				++count;
				
				// If we have run more times than there are blocks in the block manager, then there
				// is definitely something wrong.
				if (count > (int)(blocks->length() + 2))
				{
					LOG_USER(out) << "SegmentGuarantor test: too many iterations" << endl;
					return false;
				}
			} while(!needBlocks->empty());
		}
	}
	else
	{
		pipeline::Value<Blocks> needBlocks;
		
		LOG_USER(out) << "Extracting slices from entire stack" << endl;
		
		sliceGuarantor->setInput("blocks", blocks);
		segmentGuarantor->setInput("blocks", blocks);
		
		sliceGuarantor->guaranteeSlices();
		needBlocks = segmentGuarantor->guaranteeSegments();
		
		if (!needBlocks->empty())
		{
			LOG_USER(out) << "SegmentGuarantor says it needs slices for " <<
				needBlocks->length() << ", but we extracted slices for all of them" << endl;
			foreach (boost::shared_ptr<Block> block, *needBlocks)
			{
				LOG_USER(out) << "\t" << *block << endl;
			}
			return false;
		}
	}
	
	LOG_USER(out) << "Done extracting segments" << std::endl;
	
	return true;
}

void logSegment(const boost::shared_ptr<Segment> segment)
{
	LOG_USER(out) << Segment::typeString(segment->getType()) << " ";
	
	if (segment->getDirection() == Left)
	{
		LOG_USER(out) << "Left ";
	}
	else
	{
		LOG_USER(out) << "Right ";
	}
	
	foreach (boost::shared_ptr<Slice> slice, segment->getSlices())
	{
		// Use hash values rather than ids because we want to cross-reference later
		// hash values are consistent across equality, whereas slice ids can vary.
		LOG_USER(out) << slice->getSection() << ":" << slice->hashValue() << " ";
	}
	LOG_USER(out) << endl;
}

bool featureEquals(const std::vector<double>& v1, const std::vector<double>& v2)
{
	if (v1.size() == v2.size())
	{
		for(unsigned int i = 0; i < v1.size(); ++i)
		{
			if (v1[i] != v2[i])
			{
				return false;
			}
		}
		
		return true;
	}
	else
	{
		return false;
	}
}

bool testSegments(util::point3<unsigned int> stackSize, util::point3<unsigned int> blockSize)
{
	// SOPNET variables
	int i = 0;
	bool segExtraction = false, ok = true;
	std::string membranePath = optionCoreTestMembranesPath.as<std::string>();
	std::string rawPath = optionCoreTestRawImagesPath.as<std::string>();
	
	boost::shared_ptr<ImageStackDirectoryReader> directoryStackReader =
			boost::make_shared<ImageStackDirectoryReader>(membranePath);
	pipeline::Value<ImageStack> stack = directoryStackReader->getOutput();
	
	boost::shared_ptr<ImageStackDirectoryReader> rawImageStackReader =
			boost::make_shared<ImageStackDirectoryReader>(rawPath);
	boost::shared_ptr<SegmentFeaturesExtractor> featureExtractor = 
		boost::make_shared<SegmentFeaturesExtractor>();
	
	pipeline::Value<Segments> sopnetSegments = pipeline::Value<Segments>();
	pipeline::Value<Slices> prevSlices, nextSlices;
	pipeline::Value<Features> sopnetFeatures;
	
	// Extract Segments as in the original SOPNET pipeline.
	foreach (boost::shared_ptr<Image> image, *stack)
	{
		boost::shared_ptr<SliceExtractor<unsigned char> > extractor =
			boost::make_shared<SliceExtractor<unsigned char> >(i++, true);
		extractor->setInput("membrane", image);
		nextSlices = extractor->getOutput("slices");
		
		if (segExtraction)
		{
			// bfe: Bool Force Explanation
			bool bfe = optionCoreTestForceExplanation;
			pipeline::Value<bool> forceExplanation =
				pipeline::Value<bool>(bfe);
			boost::shared_ptr<SegmentExtractor> segmentExtractor =
				boost::make_shared<SegmentExtractor>();
			pipeline::Value<Segments> segments;
			
			segmentExtractor->setInput("previous slices", prevSlices);
			segmentExtractor->setInput("next slices", nextSlices);
			segmentExtractor->setInput("force explanation", forceExplanation);
			
			segments = segmentExtractor->getOutput("segments");
			sopnetSegments->addAll(segments);
		}
		
		segExtraction = true;
		prevSlices = nextSlices;
	}
	
	// Collect Features
	LOG_USER(out) << "Collecting segment features for sopnet pipeline" << std::endl;
	featureExtractor->setInput("segments", sopnetSegments);
	featureExtractor->setInput("raw sections", rawImageStackReader->getOutput());
	sopnetFeatures = featureExtractor->getOutput("all features");
	
	LOG_USER(out) << "Extracted " << sopnetFeatures->size() << " sopnet features for " <<
		sopnetSegments->size() << " segments" << std::endl;
	
	// Blockwise variables
	boost::shared_ptr<BlockManager> blockManager = boost::make_shared<LocalBlockManager>(stackSize,
																					blockSize,
																					tempCoreSize);
	boost::shared_ptr<StackStore> membraneStackStore =
		boost::make_shared<LocalStackStore>(membranePath);
	boost::shared_ptr<StackStore> rawStackStore = boost::make_shared<LocalStackStore>(rawPath);
	boost::shared_ptr<Box<> > stackBox =
		boost::make_shared<Box<> >(util::point3<unsigned int>(0,0,0), stackSize);
	boost::shared_ptr<Blocks> blocks = blockManager->blocksInBox(stackBox);
	boost::shared_ptr<SliceStore> sliceStore = boost::make_shared<LocalSliceStore>();
	boost::shared_ptr<SegmentStore> segmentStore = boost::make_shared<LocalSegmentStore>();
	
	boost::shared_ptr<SegmentReader> segmentReader = boost::make_shared<SegmentReader>();
	boost::shared_ptr<SegmentFeatureReader> segmentFeatureReader = 
		boost::make_shared<SegmentFeatureReader>();
	
	pipeline::Value<Segments> blockwiseSegments;
	pipeline::Value<Features> blockwiseFeatures;
	
	// Now, do it blockwise
	if (!guaranteeSegments(blocks, sliceStore, segmentStore, membraneStackStore, rawStackStore))
	{
		return false;
	}
	
	LOG_USER(out) << "Segment test: segments extracted successfully" << std::endl;
	
	segmentReader->setInput("blocks", blocks);
	segmentReader->setInput("store", segmentStore);
	blockwiseSegments = segmentReader->getOutput("segments");
	
	LOG_USER(out) << "read back " << blockwiseSegments->size() << " blockwise segments" <<
		std::endl;
	
	segmentFeatureReader->setInput("segments", blockwiseSegments);
	segmentFeatureReader->setInput("store", segmentStore);
	segmentFeatureReader->setInput("block manager", blocks->getManager());
	segmentFeatureReader->setInput("raw stack store", rawStackStore);
	
	blockwiseFeatures = segmentFeatureReader->getOutput("features");
	
	LOG_USER(out) << "Read " << blockwiseFeatures->size() << " features blockwise " << std::endl;
	// Now, check for differences
	
	// First, check for differences in the segment sets.
	
	// Segments that appear in the blockwise segments, but not the sopnet segments
	boost::shared_ptr<Segments> bsSegmentSetDiff = boost::make_shared<Segments>();
	// vice-versa
	boost::shared_ptr<Segments> sbSegmentSetDiff = boost::make_shared<Segments>();
	SegmentSetType sopnetSegmentSet;
	SegmentSetType blockwiseSegmentSet;
	SegmentSetType badFeatureSegmentSet; 
	
	LOG_USER(out) << "Checking for segment equality" << std::endl;
	
	foreach (boost::shared_ptr<Segment> segment, blockwiseSegments->getSegments())
	{
		blockwiseSegmentSet.insert(segment);
	}
	
	foreach (boost::shared_ptr<Segment> segment, sopnetSegments->getSegments())
	{
		sopnetSegmentSet.insert(segment);
	}
	

	foreach (boost::shared_ptr<Segment> blockWiseSegment, blockwiseSegments->getSegments())
	{
// 		if (!segmentsContains(sopnetSegments, blockWiseSegment))
		
		if (!sopnetSegmentSet.count(blockWiseSegment))
		{
			bsSegmentSetDiff->add(blockWiseSegment);
			ok = false;
		}
	}
	
	foreach (boost::shared_ptr<Segment> sopnetSegment, sopnetSegments->getSegments())
	{
		//if (!segmentsContains(blockwiseSegments, sopnetSegment))
		if (!blockwiseSegmentSet.count(sopnetSegment))
		{
			sbSegmentSetDiff->add(sopnetSegment);
			ok = false;
		}
	}
	
	
	if (ok)
	{
		LOG_USER(out) << "Segments are consistent" << std::endl;
	}
	else
	{
		LOG_USER(out) << bsSegmentSetDiff->size() <<
			" segments were found in the blockwise output but not sopnet: " << endl;
		if (bsSegmentSetDiff->size() > 0)
		{
			LOG_USER(out) << "\t" << bsSegmentSetDiff->getEnds().size() <<
				" Ends" << endl;
			LOG_USER(out) << "\t" << bsSegmentSetDiff->getContinuations().size() <<
				" Continuations" << endl;
			LOG_USER(out) << "\t" << bsSegmentSetDiff->getBranches().size() <<
				" Branches" << endl;
			
		}
		foreach (boost::shared_ptr<Segment> segment, bsSegmentSetDiff->getSegments())
		{
			logSegment(segment);
		}
		
		LOG_USER(out) << sbSegmentSetDiff->size() <<
			" segments were found in the sopnet output but not blockwise: " << endl;
		if (sbSegmentSetDiff->size() > 0)
		{
			LOG_USER(out) << "\t" << sbSegmentSetDiff->getEnds().size() <<
				" Ends" << endl;
			LOG_USER(out) << "\t" << sbSegmentSetDiff->getContinuations().size() <<
				" Continuations" << endl;
			LOG_USER(out) << "\t" << sbSegmentSetDiff->getBranches().size() <<
				" Branches" << endl;
			
		}
		foreach (boost::shared_ptr<Segment> segment, sbSegmentSetDiff->getSegments())
		{
			logSegment(segment);
		}
		
		return false;
	}
	
	
	// If the Segments test passed, check for equality in Features
	if (ok)
	{
		LOG_USER(out) << "Testing features for equality" << std::endl;
		
		LOG_USER(out) << "Sopnet features contain " << sopnetFeatures->getSegmentsIdsMap().size() <<
			" entries" << std::endl;
		LOG_USER(out) << "Blockwise features contain " <<
			blockwiseFeatures->getSegmentsIdsMap().size() << " entries" << std::endl;
		
		LOG_USER(out) << "\tand does not contain entries for the following segments:" << std::endl;
		foreach (boost::shared_ptr<Segment> blockwiseSegment, blockwiseSegmentSet)
		{
			if (!blockwiseFeatures->getSegmentsIdsMap().count(blockwiseSegment->getId()))
			{
				LOG_USER(out) << "\t" << blockwiseSegment->getId() << " " <<
					blockwiseSegment->hashValue() << std::endl;
			}
		}
		
		foreach (boost::shared_ptr<Segment> sopnetSegment, sopnetSegmentSet)
		{
			boost::shared_ptr<Segment> blockwiseSegment = *blockwiseSegmentSet.find(sopnetSegment);
			std::vector<double> sopnetFeature = sopnetFeatures->get(sopnetSegment->getId());
			std::vector<double> blockwiseFeature = blockwiseFeatures->get(blockwiseSegment->getId());
			if (!featureEquals(sopnetFeature, blockwiseFeature))
			{
				ok = false;
				badFeatureSegmentSet.insert(sopnetSegment);
			}
		}
		
		if (!ok)
		{
			LOG_USER(out) << "Features were unequal for " << badFeatureSegmentSet.size() <<
				" Segment hashes:" << std::endl;
			foreach (boost::shared_ptr<Segment> segment, badFeatureSegmentSet)
			{
				LOG_USER(out) << " " << segment->hashValue();
			}
			LOG_USER(out) << std::endl;
		}
		else
		{
			LOG_USER(out) << "Segment feature test passed" << std::endl;
		}
	}
	
	
	if (optionCoreTestWriteDebugFiles)
	{
		writeSegments(sopnetSegments, sopnetOutputPath);
		writeSegments(blockwiseSegments, blockwiseOutputPath);
	}


	if (ok)
	{
		LOG_USER(out) << "Segment test passed" << endl;
	}
	else
	{
		LOG_USER(out) << "Segment test failed" << endl;
		

	}

	return ok;
}

// bool segmentIntersectsBox(const boost::shared_ptr<Segment> segment,
// 						  const boost::shared_ptr<Box<> > box)
// {
// 	util::rect<unsigned int> boxUIRect = *box;
// 	util::rect<int> boxRect = static_cast<util::rect<int>>(boxUIRect);
// 	
// 	foreach (boost::shared_ptr<Slice> slice, segment->getSlices())
// 	{
// 		if (slice->getComponent()->getBoundingBox().intersects(boxRect))
// 		{
// 			return true;
// 		}
// 	}
// 	
// 	return false;
// }


boost::shared_ptr<Solution> readAllSolutions(const boost::shared_ptr<Cores> cores,
										   const boost::shared_ptr<SegmentStore> segmentStore,
										   const boost::shared_ptr<Segments> segments)
{
	boost::shared_ptr<Solution> solution = boost::make_shared<Solution>();
	solution->resize(segments->size());
	
	unsigned int j = 0;
	foreach (boost::shared_ptr<Segment> segment, segments->getSegments())
	{
		(*solution)[j++] = 0;
	}
	
	foreach (boost::shared_ptr<Core> core, *cores)
	{
		boost::shared_ptr<SegmentSolutionReader> solutionReader = boost::make_shared<SegmentSolutionReader>();
		pipeline::Value<Solution> coreSolution;
		
		solutionReader->setInput("core", core);
		solutionReader->setInput("store", segmentStore);
		solutionReader->setInput("segments", segments);
		coreSolution = solutionReader->getOutput("solution");
		
		for (unsigned int i = 0; i < coreSolution->size(); ++i)
		{
			if ((*coreSolution)[i] > (*solution)[i])
			{
				(*solution)[i] = (*coreSolution)[i];
			}
		}
		
	}
	
	return solution;
}

bool coreSolver(
	const boost::shared_ptr<SegmentationCostFunctionParameters> segmentationCostParameters,
	const boost::shared_ptr<PriorCostFunctionParameters> priorCostFunctionParameters,
	const boost::shared_ptr<SliceStore> sliceStore,
	const boost::shared_ptr<SegmentStore> segmentStore,
	const boost::shared_ptr<StackStore> membraneStackStore,
	const boost::shared_ptr<StackStore> rawStackStore,
	const boost::shared_ptr<BlockManager> blockManager,
	unsigned int buffer,
	boost::shared_ptr<Segments>& solutionSegmentsOut,
	boost::shared_ptr<Segments>& segmentsOut,
	boost::shared_ptr<LinearObjective>& objectiveOut)
{
	pipeline::Value<unsigned int> maxSize(1024 * 1024 * 64);

	unsigned int count = 0;
	
	// Block pipeline variables
	boost::shared_ptr<SegmentReader> segmentReader = boost::make_shared<SegmentReader>();
	boost::shared_ptr<SliceReader> sliceReader= boost::make_shared<SliceReader>();
	boost::shared_ptr<SegmentSolutionReader> solutionReader = boost::make_shared<SegmentSolutionReader>();
	boost::shared_ptr<CostReader> costReader = boost::make_shared<CostReader>();
	//boost::shared_ptr<CoreSolver> coreSolver = boost::make_shared<CoreSolver>();
	boost::shared_ptr<Reconstructor> reconstructor = boost::make_shared<Reconstructor>();
	bool bfe = optionCoreTestForceExplanation;
	pipeline::Value<bool> forceExplanation = pipeline::Value<bool>(bfe);
	pipeline::Value<unsigned int> bufferValue = pipeline::Value<unsigned int>(buffer);
	util::point3<unsigned int> stackSize = blockManager->stackSize();
	
	boost::shared_ptr<SolutionGuarantor> solutionGuarantor = 
		boost::make_shared<SolutionGuarantor>();
	
	boost::shared_ptr<Blocks> blocks;
	pipeline::Value<Blocks> needBlocks;
	boost::shared_ptr<Cores> cores = blockManager->coresInBox(
		boost::make_shared<Box<> >(util::point3<unsigned int>(0,0,0), stackSize));
	
	// Result Values
	//pipeline::Value<SliceStoreResult> sliceResult;
	pipeline::Value<SegmentTrees> neurons;
	pipeline::Value<Segments> segments;
	boost::shared_ptr<Solution> solution;
	pipeline::Value<LinearObjective> objective;
	pipeline::Value<Segments> solutionSegments;
	
	solutionGuarantor->setInput("cores", cores);
	solutionGuarantor->setInput("slice store", sliceStore);
	solutionGuarantor->setInput("segment store", segmentStore);
	solutionGuarantor->setInput("raw stack store", rawStackStore);
	solutionGuarantor->setInput("membrane stack store", membraneStackStore);
	solutionGuarantor->setInput("buffer", bufferValue);
	solutionGuarantor->setInput("force explanation", forceExplanation);
	solutionGuarantor->setInput("segmentation cost parameters", segmentationCostParameters);
	solutionGuarantor->setInput("prior cost parameters", priorCostFunctionParameters);
	
	LOG_USER(out) << "D" << endl;
	
	do
	{
		LOG_USER(out) << "Attempting to guarantee a solution" << endl;
		solutionGuarantor->setInput("segment store", segmentStore);
		
		LOG_USER(out) << "Sigh." << endl;
		needBlocks = solutionGuarantor->guaranteeSolution();
		
		LOG_USER(out) << "Need segments for " << needBlocks->length() << " blocks" << std::endl;
		
		if (!needBlocks->empty() &&
			!guaranteeSegments(needBlocks, sliceStore, segmentStore,
							   membraneStackStore, rawStackStore))
		{
			return false;
		}
		LOG_USER(out) << "end of do block" << endl;
		if (++count > 10)
		{
			return false;
		}
	} while (!needBlocks->empty());

	LOG_USER(out) << "E" << endl;
	
	segmentReader->setInput("blocks", cores->asBlocks());
	segmentReader->setInput("store", segmentStore);
	segments = segmentReader->getOutput("segments");
	
	solution = readAllSolutions(cores, segmentStore, segments);
	
// 	solutionReader->setInput("core", core);
// 	solutionReader->setInput("store", segmentStore);
// 	solutionReader->setInput("segments", segments);
// 	solution = solutionReader->getOutput("solution");
	
	costReader->setInput("store", segmentStore);
	costReader->setInput("segments", segments);
	objective = costReader->getOutput();
	
	reconstructor->setInput("solution", solution);
	reconstructor->setInput("segments", segments);
	
	solutionSegments = reconstructor->getOutput();
	
	solutionSegmentsOut->addAll(solutionSegments);
	objectiveOut = objective;
	segmentsOut->addAll(segments);
	
	return true;
}

bool overlap(boost::shared_ptr<Segment> segment, boost::shared_ptr<Box<> > box)
{
	boost::shared_ptr<Slice> slice1 = segment->getSlices()[0];
	if (slice1->getSection() >= box->location().z &&
		slice1->getSection() - box->location().z < box->size().z)
	{
		util::rect<unsigned int> boxRectUI = *box;
		util::rect<int> rectI = static_cast<util::rect<int> >(boxRectUI);
		foreach (boost::shared_ptr<Slice> slice, segment->getSlices())
		{
			if (rectI.intersects(slice->getComponent()->getBoundingBox()))
			{
				return true;
			}
		}
	}
	
	return false;
	
}


void sopnetSolver(
	const boost::shared_ptr<SegmentationCostFunctionParameters> segmentationCostParameters,
	const boost::shared_ptr<PriorCostFunctionParameters> priorCostFunctionParameters,
	const boost::shared_ptr<SliceStore>,
	const boost::shared_ptr<SegmentStore>,
	const boost::shared_ptr<StackStore> membraneStackStore,
	const boost::shared_ptr<StackStore> rawStackStore,
	const boost::shared_ptr<BlockManager> blockManager,
	unsigned int,
	boost::shared_ptr<Segments>& solutionSegmentsOut,
	boost::shared_ptr<Segments>& segmentsOut,
	boost::shared_ptr<LinearObjective>& objectiveOut)
{
	boost::shared_ptr<Sopnet> sopnet = boost::make_shared<Sopnet>("woo hoo");
	boost::shared_ptr<NeuronExtractor> neuronExtractor = boost::make_shared<NeuronExtractor>();
	
	bool bfe = optionCoreTestForceExplanation;

	pipeline::Value<SegmentTrees> neurons;
	pipeline::Value<Segments> solutionSegments, allSegments;
	pipeline::Value<bool> forceExplanation = pipeline::Value<bool>(bfe);
	pipeline::Value<LinearObjective> objective;
	boost::shared_ptr<Box<> > box = boost::make_shared<Box<> >(
		util::point3<unsigned int>(0,0,0), blockManager->stackSize());
	
	
	LOG_USER(out) << "Grabbing raw stack for box " << *box << endl;
	pipeline::Value<ImageStack> rawStack = rawStackStore->getImageStack(*box);
	LOG_USER(out) << "Grabbing membrane stack" << endl;
	pipeline::Value<ImageStack> membraneStack = membraneStackStore->getImageStack(*box);
	
	LOG_USER(out) << "Setting up inputs" << endl;
	
	sopnet->setInput("raw sections", rawStack);
	sopnet->setInput("membranes", membraneStack);
	sopnet->setInput("neuron slices", membraneStack);
	sopnet->setInput("segmentation cost parameters", segmentationCostParameters);
	sopnet->setInput("prior cost parameters", priorCostFunctionParameters);
	sopnet->setInput("force explanation", forceExplanation);
	solutionSegments = sopnet->getOutput("solution");
	objective = sopnet->getOutput("objective");
	allSegments = sopnet->getOutput("segments");
	
	objectiveOut = objective;
	solutionSegmentsOut->addAll(solutionSegments);
	segmentsOut->addAll(allSegments);
}

bool segmentTreesContains(const boost::shared_ptr<SegmentTrees> trees,
						  const boost::shared_ptr<SegmentTree> tree)
{
	foreach (boost::shared_ptr<SegmentTree> otherTree, *trees)
	{
		if (*tree == *otherTree)
		{
			return true;
		}
	}
	
	return false;
}

bool checkSegmentCosts(boost::shared_ptr<Segments> sopnetSegments,
					   boost::shared_ptr<Segments> blockwiseSegments,
					   boost::shared_ptr<LinearObjective> sopnetObjective,
					   boost::shared_ptr<LinearObjective> blockwiseObjective)
{
	bool ok = true;
	boost::unordered_map<boost::shared_ptr<Segment>, unsigned int,
		SegmentPointerHash, SegmentPointerEquals> coreSegmentIdMap;
	unsigned int i = 0;
	std::vector<double> sopnetCoefs = sopnetObjective->getCoefficients();
	std::vector<double> coreCoefs = blockwiseObjective->getCoefficients();
	
	LOG_USER(out) << "Cost testing" << endl;
	
	foreach (boost::shared_ptr<Segment> segment, blockwiseSegments->getSegments())
	{
		coreSegmentIdMap[segment] = i++;
	}
	
	i = 0;
	
	foreach (boost::shared_ptr<Segment> segment, sopnetSegments->getSegments())
	{
		if (coreSegmentIdMap.count(segment))
		{
			double sval = sopnetCoefs[i];
			double cval = coreCoefs[coreSegmentIdMap[segment]];
			if (cval != sval)
			{
				LOG_USER(out) << segment->hashValue() << " " << sval << " " << cval << endl;
				ok = false;
			}
		}
		else
		{
			LOG_USER(out) << segment->hashValue() << " -1 -1 " << std::endl;
			ok = false;
		}
	}
	
	return ok;
}

bool checkSegmentTrees(boost::shared_ptr<SegmentTrees> sopnetNeurons,
					   boost::shared_ptr<SegmentTrees> blockwiseNeurons)
{
	bool ok = true;
	// Trees that appear in the blockwise solution but not the sopnet one.
	boost::shared_ptr<SegmentTrees> bsTreeDiff = boost::make_shared<SegmentTrees>();
	// Vice-versa
	boost::shared_ptr<SegmentTrees> sbTreeDiff = boost::make_shared<SegmentTrees>();

	foreach (boost::shared_ptr<SegmentTree> blockwiseTree, *blockwiseNeurons)
	{
		if (!segmentTreesContains(sopnetNeurons, blockwiseTree))
		{
			ok = false;
			bsTreeDiff->add(blockwiseTree);
		}
	}
	
	foreach (boost::shared_ptr<SegmentTree> sopnetTree, *sopnetNeurons)
	{
		if (!segmentTreesContains(blockwiseNeurons, sopnetTree))
		{
			ok = false;
			sbTreeDiff->add(sopnetTree);
		}
	}
	
	if (!ok)
	{
		LOG_USER(out) << bsTreeDiff->size() <<
			" Neurons  were found in the blockwise output but not sopnet:" << endl;
		foreach (boost::shared_ptr<SegmentTree> tree, *bsTreeDiff)
		{
			LOG_USER(out) << "Tree with " << tree->size() << " segments:" << endl;
			foreach (boost::shared_ptr<Segment> segment, tree->getSegments())
			{
				logSegment(segment);
			}
			LOG_USER(out) << endl;
		}
		
		LOG_USER(out) << sbTreeDiff->size() <<
			" Neurons  were found in the sopnet output but not blockwise:" << endl;
		foreach (boost::shared_ptr<SegmentTree> tree, *sbTreeDiff)
		{
			LOG_USER(out) << "Tree with " << tree->size() << " segments:" << endl;
			foreach (boost::shared_ptr<Segment> segment, tree->getSegments())
			{
				logSegment(segment);
			}
			LOG_USER(out) << endl;
		}
	}
	
	return ok;
}

bool testCostIO(boost::shared_ptr<BlockManager> blockManager,
				boost::shared_ptr<Segments> segments,
				boost::shared_ptr<LinearObjective> objective,
				boost::shared_ptr<Box<> > stackBox)
{
	boost::shared_ptr<SegmentStore> segmentStore = boost::make_shared<LocalSegmentStore>();
	boost::shared_ptr<SegmentWriter> segmentWriter = boost::make_shared<SegmentWriter>();
	boost::shared_ptr<CostWriter> costWriter = boost::make_shared<CostWriter>();
	boost::shared_ptr<CostReader> costReader = boost::make_shared<CostReader>();
	boost::shared_ptr<Blocks> blocks = blockManager->blocksInBox(stackBox);
	pipeline::Value<LinearObjective> readObjective;
	std::vector<double> coefs, readCoefs;
	bool ok = true;
	int count;
	
	segmentWriter->setInput("segments", segments);
	segmentWriter->setInput("store", segmentStore);
	segmentWriter->setInput("blocks", blocks);
	segmentWriter->writeSegments();
	
	costWriter->setInput("store", segmentStore);
	costWriter->setInput("segments", segments);
	costWriter->setInput("objective", objective);
	count = costWriter->writeCosts();
	
	LOG_USER(out) << "Wrote " << count << " of " << objective->size() << " costs for " <<
		segments->size() << " segments " << endl;
	
	costReader->setInput("store", segmentStore);
	costReader->setInput("segments", segments);
	readObjective = costReader->getOutput();
	
	
	LOG_USER(out) << "Testing cost IO directly" << std::endl;
	//ok = checkSegmentCosts(segments, segments, objective, readObjective);
	
	if (objective->size() != readObjective->size())
	{
		LOG_USER(out) << "Objective sizes differ: input at " << objective->size() <<
			", returned at " << readObjective->size() << endl;
		return false;
	}
	
	coefs = objective->getCoefficients();
	readCoefs = readObjective->getCoefficients();
	
	for (unsigned int i = 0; i < objective->size(); ++i)
	{
		if (coefs[i] != readCoefs[i])
		{
			ok = false;
			LOG_USER(out) << segments->getSegments()[i]->hashValue() <<
				" " << coefs[i] << " " << readCoefs[i] << endl;
		}
	}
	
	
	if (ok)
	{
		LOG_USER(out) << "Segment cost test ok" << std::endl;
	}
	else
	{
		LOG_USER(out) << "Segment cost test FAIL" << std::endl;
	}
	
	LOG_USER(out) << "ok: " << ok << std::endl;
	
	return ok;
	
}

bool checkSolutionSegments(const boost::shared_ptr<Segments> sopnetSolutionSegments,
						   const boost::shared_ptr<Segments> blockwiseSolutionSegments,
						   const unsigned int zMax)
{
	SegmentSetType sopnetSet, blockwiseSet;
	bool ok = true;
	boost::shared_ptr<Segments> bsSegmentSetDiff, sbSegmentSetDiff;
	bsSegmentSetDiff = boost::make_shared<Segments>();
	sbSegmentSetDiff = boost::make_shared<Segments>();
	int sCount = 0, bCount = 0;
	
	foreach (boost::shared_ptr<Segment> segment, sopnetSolutionSegments->getSegments())
	{
		sopnetSet.insert(segment);
	}
	
	foreach (boost::shared_ptr<Segment> segment, blockwiseSolutionSegments->getSegments())
	{
		blockwiseSet.insert(segment);
	}
	
	// All blockwise segments should exist in the sopnet set
	foreach (boost::shared_ptr<Segment> blockwiseSegment, blockwiseSolutionSegments->getSegments())
	{
		if (!sopnetSet.count(blockwiseSegment))
		{
			         bsSegmentSetDiff->add(blockwiseSegment);
			ok = false;
		}
		++bCount;
	}
	
	// We may miss some sopnet segments in the blockwise set. This is OK as long as
	// 1) They are EndSegments and
	// 2) They are at the upper bound of the stack.
	foreach (boost::shared_ptr<Segment> sopnetSegment, sopnetSolutionSegments->getSegments())
	{
		if (!blockwiseSet.count(sopnetSegment))
		{
			bool isEnd = sopnetSegment->getType() == EndSegmentType;
			bool upperBound = sopnetSegment->getSlices()[0]->getSection() == zMax;
			         sbSegmentSetDiff->add(sopnetSegment);
			
			if (!isEnd || !upperBound)
			{
				ok = false;
			}
		}
		++sCount;
	}
	
	LOG_USER(out) << "Sopnet yielded " << sopnetSolutionSegments->size() <<
			", and blockwise " << blockwiseSolutionSegments->size() << endl;
	LOG_USER(out) << "Tested " << sCount << " sopnet segments and " <<
		bCount << " blockwise segments. Yes, this test actually happened" << endl;
	
	if (ok)
	{
		LOG_USER(out) << "Solutions Check out" << endl;
	}
	else
	{
		LOG_USER(out) << bsSegmentSetDiff->size() <<
			" segments were found in the blockwise solution but not sopnet: " << endl;

		if (bsSegmentSetDiff->size() > 0)
		{
			LOG_USER(out) << "\t" << bsSegmentSetDiff->getEnds().size() <<
				" Ends" << endl;
			LOG_USER(out) << "\t" << bsSegmentSetDiff->getContinuations().size() <<
				" Continuations" << endl;
			LOG_USER(out) << "\t" << bsSegmentSetDiff->getBranches().size() <<
				" Branches" << endl;
			
			foreach (boost::shared_ptr<Segment> segment, bsSegmentSetDiff->getSegments())
			{
				logSegment(segment);
			}
		}
		
		LOG_USER(out) << sbSegmentSetDiff->size() <<
			" segments were found in the sopnet solution but not blockwise: " << endl;
		LOG_USER(out) << "\tKeep in mind that several EndSegments from the upper stack " <<
			"bound should be missing" << endl;
		if (sbSegmentSetDiff->size() > 0)
		{
			LOG_USER(out) << "\t" << sbSegmentSetDiff->getEnds().size() <<
				" Ends" << endl;
			LOG_USER(out) << "\t" << sbSegmentSetDiff->getContinuations().size() <<
				" Continuations" << endl;
			LOG_USER(out) << "\t" << sbSegmentSetDiff->getBranches().size() <<
				" Branches" << endl;
			
			foreach (boost::shared_ptr<Segment> segment, sbSegmentSetDiff->getSegments())
			{
				logSegment(segment);
			}
		}
	}
	
	return ok;
}


bool testSolutions(util::point3<unsigned int> stackSize, util::point3<unsigned int> blockSize)
{
	bool ok = true;
	
	//TODO consider creating test object classes
	
	LOG_USER(out) << "Testing solutions" << std::endl;
	
	std::string membranePath = optionCoreTestMembranesPath.as<std::string>();
	std::string rawPath = optionCoreTestRawImagesPath.as<std::string>();
	boost::shared_ptr<StackStore> membraneStackStore = boost::make_shared<LocalStackStore>(membranePath);
	boost::shared_ptr<StackStore> rawStackStore = boost::make_shared<LocalStackStore>(rawPath);
	
	boost::shared_ptr<SegmentationCostFunctionParameters> segmentationCostParameters = 
		boost::make_shared<SegmentationCostFunctionParameters>();
	boost::shared_ptr<PriorCostFunctionParameters> priorCostFunctionParameters = 
		boost::make_shared<PriorCostFunctionParameters>();
		
	boost::shared_ptr<BlockManager> blockManager =
		boost::make_shared<LocalBlockManager>(stackSize, blockSize, tempCoreSize);
	
	boost::shared_ptr<Box<> > stackBox =
		boost::make_shared<Box<> >(util::point3<unsigned int>(0, 0, 0), stackSize);
	
	segmentationCostParameters->weightPotts = 0;
	segmentationCostParameters->weight = 0;
	segmentationCostParameters->priorForeground = 0.2;
	
	priorCostFunctionParameters->priorContinuation = -50;
	priorCostFunctionParameters->priorBranch = -100;
	
	boost::shared_ptr<Segments> sopnetSolutionSegments = boost::make_shared<Segments>();
	boost::shared_ptr<Segments> blockwiseSolutionSegments = boost::make_shared<Segments>();
	boost::shared_ptr<Segments> sopnetSegments = boost::make_shared<Segments>();
	boost::shared_ptr<Segments> blockwiseSegments = boost::make_shared<Segments>();
	boost::shared_ptr<LinearObjective> sopnetObjective = boost::make_shared<LinearObjective>();
	boost::shared_ptr<LinearObjective> blockwiseObjective =
		boost::make_shared<LinearObjective>();
	boost::shared_ptr<SliceStore> sliceStore = boost::make_shared<LocalSliceStore>();
	boost::shared_ptr<SegmentStore> segmentStore = boost::make_shared<LocalSegmentStore>();

	boost::shared_ptr<Segments> dummySegments = boost::make_shared<Segments>();
	boost::shared_ptr<LinearObjective> dummyObjective = boost::make_shared<LinearObjective>();
	
	unsigned int buffer = optionCoreBuffer.as<unsigned int>();

	LOG_USER(out) << "Test Sopnet solver" << endl;

	sopnetSolver(segmentationCostParameters, priorCostFunctionParameters,
						sliceStore, segmentStore, membraneStackStore, rawStackStore,
						blockManager, buffer,
						sopnetSolutionSegments, sopnetSegments, sopnetObjective);
	
	LOG_USER(out) << "Test Objective storage" << endl;
	
	ok &= testCostIO(blockManager, sopnetSegments, sopnetObjective, stackBox);
	
	if (!ok)
	{
		LOG_USER(out) << "Objective storage test failed, bailing on other tests" << endl;
		return false;
	}
	
	LOG_USER(out) << "Test core solver" << endl;
	
	/* Run the SolutionGuarantor twice. The first time populates the SegmentStore with the
	 * coefficients from the LinearObjectives, which is something we want to test. The second
	 * time around, the LinearObjectives are read from the store instead of resulting from
	 * a direct computation.
	 */
	
	ok &= coreSolver(segmentationCostParameters, priorCostFunctionParameters,
						sliceStore, segmentStore, membraneStackStore, rawStackStore,
						blockManager, buffer, dummySegments, dummySegments, dummyObjective);
	
	
	
	LOG_USER(out) << "Do it again!!!" << endl;
	ok &= coreSolver(segmentationCostParameters, priorCostFunctionParameters,
						sliceStore, segmentStore, membraneStackStore, rawStackStore,
						blockManager, buffer,
					blockwiseSolutionSegments, blockwiseSegments, blockwiseObjective);
	
// 	LOG_USER(out) << "Sopnet solved " << sopnetNeurons->size()
// 		<< " neurons and blockwise solved " << blockwiseNeurons->size() << "." << endl;

// 	ok = ok && checkSegmentCosts(sopnetSegments, blockwiseSegments,
// 									sopnetObjective, blockwiseObjective);
// 	ok = ok && checkSegmentTrees(sopnetNeurons, blockwiseNeurons);
// 	
// 	if (optionCoreTestWriteDebugFiles)
// 	{
// 		writeSegmentTrees(sopnetNeurons, sopnetOutputPath);
// 		writeSegmentTrees(blockwiseNeurons, blockwiseOutputPath);
// 	}
	
	checkSolutionSegments(sopnetSolutionSegments, blockwiseSolutionSegments, stackSize.z - 1);
	
	
	if (ok)
	{
		LOG_USER(out) << "Neuron solutions passed" << endl;
	}
	else
	{
		LOG_USER(out) << "Neuron solutions failed" << endl;
		return false;
	}
	
	return true;
}


util::point3<unsigned int> parseBlockSize(const util::point3<unsigned int> stackSize)
{
	std::string blockSizeFraction = optionCoreTestBlockSizeFraction.as<std::string>();
    std::string item;
	util::point3<unsigned int> blockSize;
	int num, denom;

	std::size_t slashPos = blockSizeFraction.find("/");
	
	if (slashPos == std::string::npos)
	{
		LOG_USER(out) << "Got no fraction in block size parameter, setting to stack size" <<
			endl;
		blockSize = stackSize;
	}
	else
	{
		std::string numStr = blockSizeFraction.substr(0, slashPos);
		std::string denomStr = blockSizeFraction.substr(slashPos + 1, std::string::npos);
		
		LOG_USER(out) << "Got numerator " << numStr << ", denominator " << denomStr << endl;
		num = boost::lexical_cast<int>(numStr);
		denom = boost::lexical_cast<int>(denomStr);
		
		blockSize = util::point3<unsigned int>(fractionCeiling(stackSize.x, num, denom),
										fractionCeiling(stackSize.y, num, denom),
										stackSize.z / 2 + 1);
	}

	LOG_USER(out) << "Stack size: " << stackSize << ", block size: " << blockSize << endl;
	
	return blockSize;
}

bool blockManagerCheck(util::point3<unsigned int> stackSize,
					   util::point3<unsigned int> blockSize)
{
	bool ok = true;
	boost::shared_ptr<BlockManager> blockManager =
		boost::make_shared<LocalBlockManager>(stackSize, blockSize, tempCoreSize);
	boost::shared_ptr<Block> block0 =
		blockManager->blockAtLocation(util::point3<unsigned int>(0,0,0));
	boost::shared_ptr<Block> blockNull = 
		blockManager->blockAtLocation(stackSize);
	boost::shared_ptr<Block> blockSup = 
		blockManager->blockAtLocation(stackSize - util::point3<unsigned int>(1, 1, 1));
	boost::shared_ptr<Blocks> allBlocks = blockManager->blocksInBox(
		boost::make_shared<Box<> >(util::point3<unsigned int>(0,0,0), stackSize));
	util::point3<unsigned int> maxCoords = blockManager->maximumBlockCoordinates();
	unsigned int n = maxCoords.x * maxCoords.y * maxCoords.z;

	ok &= allBlocks->length() == n;
	
	LOG_USER(out) << "Got " << allBlocks->length() << " blocks, expected " << n << endl;
	
	LOG_USER(out) << "For stack size " << stackSize << " and block size " << blockSize << ":" <<
		endl;
	LOG_USER(out) << "\tBlock 0: " << *block0 << endl;
	LOG_USER(out) << "\tBlock Sup: " << *blockSup << endl;
	if (blockNull)
	{
		LOG_USER(out) << "\tBlock inf (should be null): " << *blockNull << endl;
		ok = false;
	}
	else
	{
		LOG_USER(out) << "\tBlock inf is correctly null" << std::endl;
	}
	
	foreach (boost::shared_ptr<Block> block, *allBlocks)
	{
		if (! (*block == *blockManager->blockAtCoordinates(block->getCoordinates())))
		{
			ok = false;
			LOG_USER(out) << "Block " << *block << " with coordinates " << block->getCoordinates()
				<< " did not match block returned by the block manager for those coordinates" << endl;
		}
	}
	
	foreach (boost::shared_ptr<Block> block, *allBlocks)
	{
		if (block->getSlicesFlag())
		{
			ok = false;
			LOG_USER(out) << "Block " << *block << " slices flag instantiated to true" << endl;
		}
		block->setSlicesFlag(true);
	}
	
	foreach (boost::shared_ptr<Block> block, *allBlocks)
	{
		if (!block->getSlicesFlag())
		{
			ok = false;
			LOG_USER(out) << "Block " << *block << " slices flag false, previously set true" << endl;
		}
		if (block->getSegmentsFlag())
		{
			ok = false;
			LOG_USER(out) << "Block " << *block << " segments flag instantiated to true" << endl;
		}
		block->setSegmentsFlag(true);
	}
	
	foreach (boost::shared_ptr<Block> block, *allBlocks)
	{
		if (!block->getSegmentsFlag())
		{
			ok = false;
			LOG_USER(out) << "Block " << *block << " segments flag false, previously set true" << endl;
		}
		if (block->getSolutionCostFlag())
		{
			ok = false;
			LOG_USER(out) << "Block " << *block << " solutions flag instantiated to true" << endl;
		}
		block->setSolutionCostFlag(true);
	}

	foreach (boost::shared_ptr<Block> block, *allBlocks)
	{
		if (!block->getSolutionCostFlag())
		{
			ok = false;
			LOG_USER(out) << "Block " << *block << " solutions flag false, previously set true" << endl;
		}
	}

	
	if (ok)
	{
		LOG_USER(out) << "Block manager test passed" << endl;
	}
	else
	{
		LOG_USER(out) << "Block manager test failed" << endl;
	}
	
	return ok;
}

bool basicBlockManagerTest()
{
	bool ok = true;
	ok &= blockManagerCheck(util::point3<unsigned int>(25, 25, 10),
							util::point3<unsigned int>(10, 10, 10));
	
	ok &= blockManagerCheck(util::point3<unsigned int>(31, 31, 11),
							util::point3<unsigned int>(10, 10, 5));
	
	ok &= blockManagerCheck(util::point3<unsigned int>(30, 30, 10),
							util::point3<unsigned int>(10, 10, 5));
	
	if (!ok)
	{
		LOG_USER(out) << "Found irregularities with respect to block management" << endl;
	}
	return ok;
}

int main(int optionc, char** optionv)
{
	util::ProgramOptions::init(optionc, optionv);
	LogManager::init();
	
	try
	{
		std::string membranePath = optionCoreTestMembranesPath.as<std::string>();
		pipeline::Value<ImageStack> testStack;
		unsigned int nx, ny, nz;
		util::point3<unsigned int> stackSize, blockSize;
		
		boost::shared_ptr<ImageStackDirectoryReader> directoryStackReader =
			boost::make_shared<ImageStackDirectoryReader>(membranePath);
		
		testStack = directoryStackReader->getOutput();
		nx = testStack->width();
		ny = testStack->height();
		nz = testStack->size();
		
		stackSize = point3<unsigned int>(nx, ny, nz);
		
		blockSize = parseBlockSize(stackSize);
		
		if (!basicBlockManagerTest())
		{
			return -10;
		}
		
		if (optionCoreTestWriteSliceImages || optionCoreTestWriteDebugFiles)
		{
			LOG_USER(out) << "Creating output directories" << endl;
			blockwiseOutputPath = blockwiseOutputPath + "_" +
				boost::lexical_cast<std::string>(stackSize.x) + "x" + 
				boost::lexical_cast<std::string>(stackSize.y) + "_" + 
				boost::lexical_cast<std::string>(blockSize.x) + "x" +
				boost::lexical_cast<std::string>(blockSize.y);
			sopnetOutputPath = sopnetOutputPath + "_" +
				boost::lexical_cast<std::string>(stackSize.x) + "x" + 
				boost::lexical_cast<std::string>(stackSize.y);
			mkdir(sopnetOutputPath);
			mkdir(blockwiseOutputPath);
		}
		
		testStack->clear();
		
		if (!optionCoreTestDisableSliceTest &&
			!testSlices(stackSize, blockSize))
		{
			
			return -1;
		}
		
		if (!optionCoreTestDisableSegmentTest &&
			!testSegments(stackSize, blockSize))
		{
			return -2;
		}
		
		if (!optionCoreTestDisableSolutionTest &&
			!testSolutions(stackSize, blockSize))
		{
			return -3;
		}
		
		return 0;
	}
	catch (Exception& e)
	{
		handleException(e);
	}
}
