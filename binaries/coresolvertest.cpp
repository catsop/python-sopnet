#include <string>
#include <fstream>
#include <iostream>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem.hpp>

#include <imageprocessing/ImageStack.h>
#include <util/Logger.h>
#include <util/point3.hpp>
#include <util/ProgramOptions.h>
#include <pipeline/all.h>
#include <pipeline/Value.h>
#include <pipeline/Process.h>
#include <sopnet/Sopnet.h>
#include <sopnet/inference/PriorCostFunctionParameters.h>
#include <sopnet/inference/SegmentationCostFunctionParameters.h>
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
#include <catmaid/persistence/LocalSegmentStore.h>
#include <catmaid/persistence/LocalSliceStore.h>
#include <catmaid/persistence/LocalStackStore.h>
#include <catmaid/persistence/SegmentReader.h>
#include <sopnet/segments/SegmentSet.h>

#include <sopnet/block/Box.h>
#include <vigra/impex.hxx>
#include <sopnet/slices/SliceExtractor.h>
#include <gui/Window.h>
#include <imageprocessing/gui/ImageStackView.h>
#include <neurons/NeuronExtractor.h>

using util::point3;
using namespace logger;
using namespace std;

logger::LogChannel coretestlog("coretestlog", "[CoreTest] ");

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
		"_" + boost::lexical_cast<std::string>(id) + "_" + boost::lexical_cast<std::string>(bbox.minX) +
		"_" + boost::lexical_cast<std::string>(bbox.minY) + ".png";

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


void sopnetSolver(const std::string& membranePath, const std::string& rawPath,
	const boost::shared_ptr<SegmentationCostFunctionParameters>& segmentationCostParameters,
	const boost::shared_ptr<PriorCostFunctionParameters>& priorCostFunctionParameters,
	const boost::shared_ptr<pipeline::Wrap<bool> >& forceExplanation,
	const boost::shared_ptr<Segments>& segmentsOut,
	const boost::shared_ptr<SegmentTrees>& neuronsOut)
{
	boost::shared_ptr<ImageStackDirectoryReader> membraneReader =
		boost::make_shared<ImageStackDirectoryReader>(membranePath);
	boost::shared_ptr<ImageStackDirectoryReader> rawReader =
		boost::make_shared<ImageStackDirectoryReader>(rawPath);
	boost::shared_ptr<Sopnet> sopnet = boost::make_shared<Sopnet>("woo hoo");
	boost::shared_ptr<NeuronExtractor> neuronExtractor = boost::make_shared<NeuronExtractor>();
	
	pipeline::Value<SegmentTrees> neurons;
	pipeline::Value<Segments> segments;
	
	sopnet->setInput("raw sections", rawReader->getOutput());
	sopnet->setInput("membranes", membraneReader->getOutput());
	sopnet->setInput("neuron slices", membraneReader->getOutput());
	sopnet->setInput("segmentation cost parameters", segmentationCostParameters);
	sopnet->setInput("prior cost parameters", priorCostFunctionParameters);
	sopnet->setInput("force explanation", forceExplanation);
	neuronExtractor->setInput("segments", sopnet->getOutput("solution"));
	
	neurons = neuronExtractor->getOutput();
	segments = sopnet->getOutput("segments");
	
	LOG_USER(out) << "Solved " << neurons->size() << " neurons" << std::endl;
	
	segmentsOut->addAll(segments);
	neuronsOut->addAll(neurons);
}

void mockGuaranteePipeline(const boost::shared_ptr<Blocks> blocks,
						   const boost::shared_ptr<StackStore> stackStore,
						   const boost::shared_ptr<SliceStore> sliceStore,
						   const boost::shared_ptr<SegmentStore> segmentStore)
{
	bool done = false;
	boost::shared_ptr<SliceGuarantor> sliceGuarantor = boost::make_shared<SliceGuarantor>();
	boost::shared_ptr<SegmentGuarantor> segmentGuarantor = boost::make_shared<SegmentGuarantor>();

	
	while (!done)
	{
		pipeline::Value<Blocks> segmentNeedBlocks;
		pipeline::Value<Blocks> sliceNeedBlocks;
		
		segmentGuarantor->setInput("blocks", blocks);
		segmentGuarantor->setInput("segment store", segmentStore);
		segmentGuarantor->setInput("slice store", sliceStore);

		segmentNeedBlocks = segmentGuarantor->guaranteeSegments();
		
		if (segmentNeedBlocks->empty())
		{
			LOG_USER(out) << "Segments are guaranteed" << std::endl;
			done = true;
		}
		else
		{
			LOG_USER(out) << "Segment guarantor said it needs slices from " <<
				*segmentNeedBlocks << std::endl;
			sliceGuarantor->setInput("blocks", segmentNeedBlocks);
			sliceGuarantor->setInput("slice store", sliceStore);
			sliceGuarantor->setInput("stack store", stackStore);
			
			sliceNeedBlocks = sliceGuarantor->guaranteeSlices();
			
			if (!sliceNeedBlocks->empty())
			{
				LOG_USER(out) << "Slice gaurantor says it couldn't extract from blocks in " <<
					*sliceNeedBlocks << std::endl;
				return;
			}
			
		}
	}
}

void coreSolver(const std::string& membranePath, const std::string& rawPath,
	const boost::shared_ptr<SegmentationCostFunctionParameters>& segmentationCostParameters,
	const boost::shared_ptr<PriorCostFunctionParameters>& priorCostFunctionParameters,
	const boost::shared_ptr<pipeline::Wrap<bool> >& forceExplanation,
	const boost::shared_ptr<Segments>& segmentsOut,
	const boost::shared_ptr<SegmentTrees>& neuronsOut,
	
	const util::point3<unsigned int>& stackSize,
	const util::point3<unsigned int>& blockSize,
	const bool extractSimultaneousBlocks)
{

	boost::shared_ptr<BlockManager> blockManager =
		boost::make_shared<LocalBlockManager>(stackSize, blockSize);
	boost::shared_ptr<Box<> > box =
		boost::make_shared<Box<> >(util::point3<unsigned int>(0u, 0u, 0u), stackSize);
	pipeline::Value<unsigned int> maxSize(1024 * 1024 * 64);
	boost::shared_ptr<Blocks> blocks = blockManager->blocksInBox(box);
	
	// Block pipeline variables
	boost::shared_ptr<StackStore> membraneStackStore = boost::make_shared<LocalStackStore>(membranePath);
	boost::shared_ptr<StackStore> rawStackStore = boost::make_shared<LocalStackStore>(rawPath);
	boost::shared_ptr<LocalSliceStore> localSliceStore =
		boost::shared_ptr<LocalSliceStore>(new LocalSliceStore());
	boost::shared_ptr<SliceStore> sliceStore = localSliceStore;
	boost::shared_ptr<SegmentStore> segmentStore =
		boost::shared_ptr<SegmentStore>(new LocalSegmentStore());
	boost::shared_ptr<SegmentReader> segmentReader = boost::make_shared<SegmentReader>();
	boost::shared_ptr<SliceReader> sliceReader= boost::make_shared<SliceReader>();
	boost::shared_ptr<CoreSolver> coreSolver = boost::make_shared<CoreSolver>();
	
	// Result Values
	//pipeline::Value<SliceStoreResult> sliceResult;
	pipeline::Value<SegmentTrees> neurons;
	pipeline::Value<Segments> segments;
	pipeline::Value<Slices> slices;
	
	// pipeline



	if (extractSimultaneousBlocks)
	{
		mockGuaranteePipeline(blocks, membraneStackStore, sliceStore, segmentStore);
	}
	else
	{
		foreach (const boost::shared_ptr<Block> block, *blocks)
		{
			boost::shared_ptr<Blocks> singleBlock = blockManager->blocksInBox(block);
			mockGuaranteePipeline(singleBlock, membraneStackStore, sliceStore, segmentStore);
		}
	}
	
	LOG_USER(out) << "Setting up block solver" << std::endl;
	coreSolver->setInput("prior cost parameters",
							priorCostFunctionParameters);
	coreSolver->setInput("segmentation cost parameters", segmentationCostParameters);
	coreSolver->setInput("blocks", blocks);
	coreSolver->setInput("segment store", segmentStore);
	coreSolver->setInput("slice store", sliceStore);
	coreSolver->setInput("raw image store", rawStackStore);
	coreSolver->setInput("membrane image store", membraneStackStore);
	coreSolver->setInput("force explanation", forceExplanation);
	
	LOG_USER(out) << "Inputs are set" << endl;
	
	neurons = coreSolver->getOutput("neurons");
	segments = coreSolver->getOutput("segments");
	
	LOG_USER(out) << "Solved " << neurons->size() << " neurons" << std::endl;
	
	segmentsOut->addAll(segments);
	neuronsOut->addAll(neurons);
	
	localSliceStore->dumpStore();
	
// 	writeSliceImages(sliceStore, blocks);
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

bool segmentVectorContains(vector<boost::shared_ptr<Segment> >& segments, const boost::shared_ptr<Segment> segment)
{
	foreach (boost::shared_ptr<Segment> vSeg, segments)
	{
		if (*segment == *vSeg)
		{
			return true;
		}
	}
	
	return false;
}

bool segmentSetContains(SegmentSet& segments, const boost::shared_ptr<Segment> segment)
{
// 	foreach (boost::shared_ptr<Segment> vSeg, segments)
// 	{
// 		if (*segment == *vSeg)
// 		{
// 			return true;
// 		}
// 	}
// 	
// 	return false;

	return segments.contains(segment);
}

void writeSegment(ofstream& file, const boost::shared_ptr<Segment> segment, const string& prefix)
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
	
	file << prefix.c_str() << " ";
	
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

void compareSegments(const boost::shared_ptr<Segments> segments1,
					 const boost::shared_ptr<Segments> segments2,
					 std::string path)
{
	vector<boost::shared_ptr<Segment> > segmentVector1 = segments1->getSegments();
	vector<boost::shared_ptr<Segment> > segmentVector2 = segments2->getSegments();
	
	LOG_USER(out) << "Writing segment comparison to " << path << std::endl;
	
	ofstream file;
	file.open(path.c_str());
	
	foreach (boost::shared_ptr<Segment> segment, segmentVector1)
	{
		writeSegment(file, segment, "-1");
	}
	
	foreach (boost::shared_ptr<Segment> segment, segmentVector2)
	{
		writeSegment(file, segment, "-2");
	}
	
	foreach (boost::shared_ptr<Segment> segment, segmentVector1)
	{
		if (!segmentVectorContains(segmentVector2, segment))
		{
			writeSegment(file, segment, "0");
		}
	}
	
	foreach (boost::shared_ptr<Segment> segment, segmentVector2)
	{
		if (! segmentVectorContains(segmentVector1, segment))
		{
			writeSegment(file, segment, "1");
		}
	}
	
	file.close();
}

void compareResults(const std::string& title,
					const boost::shared_ptr<Segments> sopnetSegments,
					const boost::shared_ptr<SegmentTrees> sopnetNeurons,
					const boost::shared_ptr<Segments> blockSegments,
					const boost::shared_ptr<SegmentTrees> blockNeurons,
					int exp)
{
	std::string path;
	
	LOG_USER(out) << "For " << title << ": (Sopnet / Core)" << endl;
	LOG_USER(out) << "Segments:" << endl;
	LOG_USER(out) << "    Ends:          " << sopnetSegments->getEnds().size() << "\t" <<
		blockSegments->getEnds().size() << endl;
	LOG_USER(out) << "    Continuations: " << sopnetSegments->getContinuations().size() << "\t" <<
		blockSegments->getContinuations().size() << endl;
	LOG_USER(out) << "    Branches:      " << sopnetSegments->getBranches().size() << "\t" <<
		blockSegments->getBranches().size() << endl;
	LOG_USER(out) << "Neurons: " << sopnetNeurons->size() << "\t" << blockNeurons->size() << endl;

	path = "./experiment_" + boost::lexical_cast<std::string>(exp) + "/sopnet_block_seg_diff.txt";
	
	compareSegments(sopnetSegments, blockSegments, path);
	
}

void testSliceGuarantor(const std::string& membranePath,
						const util::point3<unsigned int>& stackSize,
						const util::point3<unsigned int>& blockSize)
{
	boost::shared_ptr<SliceGuarantor> sliceGuarantor = boost::make_shared<SliceGuarantor>();
	boost::shared_ptr<BlockManager> blockManager =
		boost::make_shared<LocalBlockManager>(stackSize, blockSize);
	boost::shared_ptr<Box<> > box =
		boost::make_shared<Box<> >(util::point3<unsigned int>(0u, 0u, 0u), stackSize);
	pipeline::Value<unsigned int> maxSize(1024 * 1024 * 64);
	boost::shared_ptr<Blocks> blocks = blockManager->blocksInBox(box);
	pipeline::Value<Blocks> needBlocks;
	
	boost::shared_ptr<LocalSliceStore> localSliceStore =
		boost::make_shared<LocalSliceStore>();
	boost::shared_ptr<SliceStore> sliceStore = localSliceStore;
	
	boost::shared_ptr<StackStore> stackStore = boost::make_shared<LocalStackStore>(membranePath);

	boost::shared_ptr<SliceReader> sliceReader = boost::make_shared<SliceReader>();
	pipeline::Value<Slices> readSlices;
	
	sliceGuarantor->setInput("slice store", sliceStore);
	sliceGuarantor->setInput("stack store", stackStore);
	
	foreach (boost::shared_ptr<Block> block, *blocks)
	{
		boost::shared_ptr<Blocks> singleBlock = blockManager->blocksInBox(block);
		sliceGuarantor->setInput("blocks", singleBlock);
		
		LOG_USER(out) << "Block: " << *singleBlock << endl;
		LOG_USER(out) << "Slice guarantor pipeline initialized, guaranteeing slices..." << endl;
		
		needBlocks = sliceGuarantor->guaranteeSlices();
	
		LOG_USER(out) << "guaranteeSlices() returned " << needBlocks->size() << " blocks." << endl;
	}
	
// 	pipeline::Process<gui::Window> window("test");
// 	pipeline::Process<ImageStackView> membraneView;
// 	window->setInput(membraneView->getOutput());
// 	membraneView->setInput(stackStore->getImageStack(*blocks));
// 	window->processEvents();
	
	sliceReader->setInput("blocks", blocks);
	sliceReader->setInput("store", sliceStore);
	
	readSlices = sliceReader->getOutput("slices");
	
	LOG_USER(out) << "Read " << readSlices->size() << " slices" << std::endl;
	
// 	writeSliceImages(sliceStore, blocks);
	
	//localSliceStore->dumpStore();
	
}

void testSolver(const std::string& membranePath,
					const std::string& rawPath,
					const util::point3<unsigned int>& stackSize)
{
	util::point3<unsigned int>  blockSize50, blockSize40;
	boost::shared_ptr<SegmentationCostFunctionParameters> segmentationCostParameters = 
		boost::make_shared<SegmentationCostFunctionParameters>();
	boost::shared_ptr<PriorCostFunctionParameters> priorCostFunctionParameters = 
		boost::make_shared<PriorCostFunctionParameters>();
	pipeline::Value<bool> yep(true);
	
	boost::shared_ptr<Segments> sopnetSegments = boost::make_shared<Segments>();
	boost::shared_ptr<Segments> coreSolverSegments = boost::make_shared<Segments>();
	boost::shared_ptr<Segments> coreSolverSegmentsBlock = boost::make_shared<Segments>();
	boost::shared_ptr<Segments> coreSolverSegmentsSequential = boost::make_shared<Segments>();
	boost::shared_ptr<Segments> coreSolverSegmentsOvlpBound = boost::make_shared<Segments>();
	boost::shared_ptr<Segments> coreSolverSegmentsOvlpBoundSequential = boost::make_shared<Segments>();
	
	boost::shared_ptr<SegmentTrees> sopnetNeurons = boost::make_shared<SegmentTrees>();
	boost::shared_ptr<SegmentTrees> coreSolverNeurons = boost::make_shared<SegmentTrees>();
	boost::shared_ptr<SegmentTrees> coreSolverNeuronsBlock = boost::make_shared<SegmentTrees>();
	boost::shared_ptr<SegmentTrees> coreSolverNeuronsSequential = boost::make_shared<SegmentTrees>();
	boost::shared_ptr<SegmentTrees> coreSolverNeuronsOvlpBound = boost::make_shared<SegmentTrees>();
	boost::shared_ptr<SegmentTrees> coreSolverNeuronsOvlpBoundSequential = boost::make_shared<SegmentTrees>();
	
	blockSize50 = util::point3<unsigned int>(fractionCeiling(stackSize.x, 1, 4),
											 fractionCeiling(stackSize.y, 1, 4), stackSize.z);
	// Blocks of this size mean that the blocks don't fit exactly within stack boundaries.
	// The result should not change in this case.
	blockSize40 = util::point3<unsigned int>(fractionCeiling(stackSize.x, 2, 5),
											 fractionCeiling(stackSize.y, 2, 5), stackSize.z);
	
	segmentationCostParameters->weightPotts = 0;
	segmentationCostParameters->weight = 0;
	segmentationCostParameters->priorForeground = 0.2;
	
	priorCostFunctionParameters->priorContinuation = -50;
	priorCostFunctionParameters->priorBranch = -100;
	
	LOG_USER(out) << "Stack size: " << stackSize << endl;
	
	LOG_USER(out) << "SOPNET SOLVING POWER!!!!" << endl;
	
	sopnetSolver(membranePath, rawPath,
		segmentationCostParameters,
		priorCostFunctionParameters,
		yep,
		sopnetSegments,
		sopnetNeurons);
	
	LOG_USER(out) << "CORE SOLVING POWER!!!! With special full-stack magic" << endl;
	
	coreSolver(membranePath, rawPath,
		segmentationCostParameters,
		priorCostFunctionParameters,
		yep,
		coreSolverSegments,
		coreSolverNeurons,
		stackSize,
		stackSize,
		true);
	++experiment;
	
	LOG_USER(out) << "CORE SOLVING POWER!!!! With special multi-block magic" << endl;
	
	coreSolver(membranePath, rawPath,
		segmentationCostParameters,
		priorCostFunctionParameters,
		yep,
		coreSolverSegmentsBlock,
		coreSolverNeuronsBlock,
		stackSize,
		blockSize50,
		true);
	++experiment;

	
	LOG_USER(out) << "CORE SOLVING POWER!!!! With special sequential-block magic" << endl;
	
	coreSolver(membranePath, rawPath,
		segmentationCostParameters,
		priorCostFunctionParameters,
		yep,
		coreSolverSegmentsSequential,
		coreSolverNeuronsSequential,
		stackSize,
		blockSize50,
		false);
	++experiment;
	
	LOG_USER(out) << "CORE SOLVING POWER!!!! With special boundary-overlapping magic" << endl;
	
	coreSolver(membranePath, rawPath,
		segmentationCostParameters,
		priorCostFunctionParameters,
		yep,
		coreSolverSegmentsOvlpBound,
		coreSolverNeuronsOvlpBound,
		stackSize,
		blockSize40,
		true);
	++experiment;
	
	LOG_USER(out) << "CORE SOLVING POWER!!!! With special sequential-boundary-overlapping magic" << endl;
	
	coreSolver(membranePath, rawPath,
		segmentationCostParameters,
		priorCostFunctionParameters,
		yep,
		coreSolverSegmentsOvlpBoundSequential,
		coreSolverNeuronsOvlpBoundSequential,
		stackSize,
		blockSize40,
		false);
	++experiment;

	
	compareResults("full stack", sopnetSegments, sopnetNeurons, coreSolverSegments, coreSolverNeurons, 0);
	compareResults("half blocks", sopnetSegments, sopnetNeurons, coreSolverSegmentsBlock, coreSolverNeuronsBlock, 1);
	compareResults("sequential half blocks", sopnetSegments, sopnetNeurons, coreSolverSegmentsSequential, coreSolverNeuronsSequential, 2);
	compareResults("40% blocks", sopnetSegments, sopnetNeurons, coreSolverSegmentsOvlpBound, coreSolverNeuronsOvlpBound, 3);
	compareResults("sequential 40% blocks", sopnetSegments, sopnetNeurons, coreSolverSegmentsOvlpBoundSequential, coreSolverNeuronsOvlpBoundSequential, 4);

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
	
	// Extract Slices as in the original SOPNET pipeline.
	foreach (boost::shared_ptr<Image> image, *stack)
	{
		boost::shared_ptr<SliceExtractor<unsigned char> > extractor =
			boost::make_shared<SliceExtractor<unsigned char> >(i++);
		pipeline::Value<Slices> slices;
		pipeline::Value<ConflictSets> conflictSets;
		extractor->setInput("membrane", image);
		slices = extractor->getOutput("slices");
		conflictSets = extractor->getOutput("conflict sets");
		
		sopnetSlices->addAll(*slices);
		sopnetConflictSets->addAll(*conflictSets);
	}
	
	// Blockwise variables
	boost::shared_ptr<SliceGuarantor> sliceGuarantor = boost::make_shared<SliceGuarantor>();
	boost::shared_ptr<BlockManager> blockManager = boost::make_shared<LocalBlockManager>(stackSize,
																					blockSize);
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
			
			LOG_DEBUG(coretestlog) << "Extracting slices from block " << *block << endl;
			
			needBlocks = sliceGuarantor->guaranteeSlices();
			
			if (!needBlocks->empty())
			{
				LOG_DEBUG(coretestlog) << "SliceGuarantor needs images for block " <<
					*block << endl;
				return false;
			}
		}
	}
	else
	{
		pipeline::Value<Blocks> needBlocks;
		
		LOG_DEBUG(coretestlog) << "Extracting slices from entire stack" << endl;
		
		sliceGuarantor->setInput("blocks", blocks);
		
		needBlocks = sliceGuarantor->guaranteeSlices();
		
		if (!needBlocks->empty())
		{
			LOG_DEBUG(coretestlog) << "SliceGuarantor needs images for " <<
				needBlocks->length() << " blocks:" << endl;
			foreach (boost::shared_ptr<Block> block, *needBlocks)
			{
				LOG_DEBUG(coretestlog) << "\t" << *block << endl;
			}
			return false;
		}
	}
	
	sliceReader->setInput("blocks", blocks);
	sliceReader->setInput("store", sliceStore);
	
	blockwiseSlices = sliceReader->getOutput("slices");
	blockwiseConflictSets = sliceReader->getOutput("conflict sets");
	
	// Compare outputs
	bool ok;
	// Maps blockwise ids to sopnet ids.
	std::map<unsigned int, unsigned int> blockwiseSopnetIDMap;
	// Slices in blockwiseSlices that are not in sopnetSlices
	boost::shared_ptr<Slices> bsSlicesSetDiff = boost::make_shared<Slices>();
	// vice-versa
	boost::shared_ptr<Slices> sbSlicesSetDiff = boost::make_shared<Slices>();
	boost::shared_ptr<ConflictSets> bsConflictSetDiff = boost::make_shared<ConflictSets>();
	boost::shared_ptr<ConflictSets> sbConflictSetDiff = boost::make_shared<ConflictSets>();
	boost::shared_ptr<ConflictSets> blockwiseConflictSetsSopnetIds;
	
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
		LOG_DEBUG(coretestlog) << "Slices found in Blockwise output but not Sopnet:" << endl;
		foreach (boost::shared_ptr<Slice> slice, *bsSlicesSetDiff)
		{
			LOG_DEBUG(coretestlog) << slice->getId() << ", " << slice->hashValue() << endl;
		}
		
		LOG_DEBUG(coretestlog) << "Slices found in Sopnet output but not Blockwise:" << endl;
		foreach (boost::shared_ptr<Slice> slice, *sbSlicesSetDiff)
		{
			LOG_DEBUG(coretestlog) << slice->getId() << ", " << slice->hashValue() << endl;
		}
	}
	
	if (ok)
	{
		blockwiseConflictSetsSopnetIds = 
			mapConflictSets(blockwiseConflictSets, blockwiseSopnetIDMap);
		LOG_DEBUG(coretestlog) << "Blockwise conflict sets mapped to sopnet ids" << std::endl;
		
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
			LOG_DEBUG(coretestlog) << "ConflictSets found in Blockwise output but not Sopnet:" << endl;
			foreach (ConflictSet conflictSet, *bsConflictSetDiff)
			{
				LOG_DEBUG(coretestlog) << conflictSet << endl;
			}
			
			LOG_DEBUG(coretestlog) << "ConflictSets found in Sopnet output but not Blockwise:" << endl;
			foreach (ConflictSet conflictSet, *sbConflictSetDiff)
			{
				LOG_DEBUG(coretestlog) << conflictSet << endl;
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
		LOG_USER(coretestlog) << "Slice test failed" << endl;
	}
	else
	{
		LOG_DEBUG(coretestlog) << "Slice test passed" << endl;
	}
	
	return ok;
}

bool testSegments(util::point3<unsigned int> stackSize, util::point3<unsigned int> blockSize)
{
	// SOPNET variables
	int i = 0;
	bool segExtraction = false;
	std::string membranePath = optionCoreTestMembranesPath.as<std::string>();
	
	boost::shared_ptr<ImageStackDirectoryReader> directoryStackReader =
			boost::make_shared<ImageStackDirectoryReader>(membranePath);
	pipeline::Value<ImageStack> stack = directoryStackReader->getOutput();
	
	pipeline::Value<Segments> sopnetSegments = pipeline::Value<Segments>();
	pipeline::Value<Slices> prevSlices, nextSlices;
	
	// Extract Segments as in the original SOPNET pipeline.
	foreach (boost::shared_ptr<Image> image, *stack)
	{
		boost::shared_ptr<SliceExtractor<unsigned char> > extractor =
			boost::make_shared<SliceExtractor<unsigned char> >(i++);
		extractor->setInput("membrane", image);
		nextSlices = extractor->getOutput("slices");
		
		if (segExtraction)
		{
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
		LOG_ALL(coretestlog) << "Got no fraction in block size parameter, setting to stack size" <<
			endl;
		blockSize = stackSize;
	}
	else
	{
		std::string numStr = blockSizeFraction.substr(0, slashPos);
		std::string denomStr = blockSizeFraction.substr(slashPos + 1, std::string::npos);
		
		LOG_ALL(coretestlog) << "Got numerator " << numStr << ", denominator " << denomStr << endl;
		num = boost::lexical_cast<int>(numStr);
		denom = boost::lexical_cast<int>(denomStr);
		
		blockSize = util::point3<unsigned int>(fractionCeiling(stackSize.x, num, denom),
										fractionCeiling(stackSize.y, num, denom),
										stackSize.z);
	}

	LOG_DEBUG(coretestlog) << "Stack size: " << stackSize << ", block size: " << blockSize << endl;
	
	return blockSize;
}

void mkdir(const std::string& path)
{
	boost::filesystem::path dir(path);
	
	boost::filesystem::create_directory(dir);

}

int main(int optionc, char** optionv)
{
	util::ProgramOptions::init(optionc, optionv);
	LogManager::init();

	/*
	 * Assumes membrane path is ./membranes and raw image path is ./raw 
	 * Requires a file ./segment_rf.hdf compatible with these images
	 */
	
	try
	{
		std::string membranePath = optionCoreTestMembranesPath.as<std::string>();
		pipeline::Value<ImageStack> testStack;
		unsigned int nx, ny, nz;
		util::point3<unsigned int> stackSize, blockSize;
		
		boost::shared_ptr<ImageStackDirectoryReader> directoryStackReader =
			boost::make_shared<ImageStackDirectoryReader>(membranePath);
		
		if (optionCoreTestWriteSliceImages || optionCoreTestWriteDebugFiles)
		{
			mkdir(sopnetOutputPath);
			mkdir(blockwiseOutputPath);
		}
		
		testStack = directoryStackReader->getOutput();
		nx = testStack->width();
		ny = testStack->height();
		nz = testStack->size();
		
		stackSize = point3<unsigned int>(nx, ny, nz);
		
		blockSize = parseBlockSize(stackSize);
		
		testStack->clear();
		
		if (!testSlices(stackSize, blockSize))
		{
			
			return -1;
		}
		
		if (!testSegments(stackSize, blockSize))
		{
			return -2;
		}
		
// 		if (!testSolutions(stackSize, blockSize))
// 		{
// 			return -3;
// 		}
		
		return 0;
	}
	catch (Exception& e)
	{
		handleException(e);
	}
}
