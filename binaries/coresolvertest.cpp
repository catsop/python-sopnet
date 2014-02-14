#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem.hpp>

#include <imageprocessing/ImageStack.h>
#include <util/Logger.h>
#include <util/point3.hpp>
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

#include <sopnet/block/Box.h>
#include <vigra/impex.hxx>
#include <sopnet/slices/SliceExtractor.h>
#include <gui/Window.h>
#include <imageprocessing/gui/ImageStackView.h>
#include <neurons/NeuronExtractor.h>

using util::point3;
using std::cout;
using std::endl;
using namespace logger;


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


void writeSliceImage(const Slice& slice, const std::string& sliceImageDirectory) {

	unsigned int section = slice.getSection();
	unsigned int id      = slice.getId();
	util::rect<int> bbox = slice.getComponent()->getBoundingBox();

	std::string filename = sliceImageDirectory + "/" + boost::lexical_cast<std::string>(section) +
		"_" + boost::lexical_cast<std::string>(id) + "_" + boost::lexical_cast<std::string>(bbox.minX) +
		"_" + boost::lexical_cast<std::string>(bbox.minY) + ".png";

	vigra::exportImage(vigra::srcImageRange(slice.getComponent()->getBitmap()),
					   vigra::ImageExportInfo(filename.c_str()));
}


void writeAllSlices(const boost::shared_ptr<ImageStack>& stack,
		const boost::shared_ptr<pipeline::Wrap<bool> >& /*forceExplanation*/)
{
	int i = 0;
	
	foreach (boost::shared_ptr<Image> image, *stack)
	{
		boost::shared_ptr<SliceExtractor<unsigned char> > extractor =
			boost::make_shared<SliceExtractor<unsigned char> >(i);
		pipeline::Value<Slices> slices;
		extractor->setInput("membrane", image);
		slices = extractor->getOutput("slices");
		foreach (boost::shared_ptr<Slice> slice, *slices)
		{
			writeSliceImage(*slice, "./true_slices");
		}
	}
}

void writeSliceImages(const boost::shared_ptr<SliceStore>& store,
					  const boost::shared_ptr<Blocks>& blocks)
{
	std::string path = "./experiment_" + boost::lexical_cast<std::string>(experiment++);
	boost::filesystem::path dir(path);
	pipeline::Value<Slices> slices;
	boost::shared_ptr<SliceReader> reader = boost::make_shared<SliceReader>();
	
	boost::filesystem::create_directory(dir);
	
	reader->setInput("store", store);
	reader->setInput("blocks", blocks);
	slices = reader->getOutput("slices");
	
	foreach (boost::shared_ptr<Slice>& slice, *slices)
	{
		writeSliceImage(*slice, path);
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
	
	//localSliceStore->dumpStore();
	
	//writeSliceImages(sliceStore, blockManager);
	
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

void compareResults(const std::string& title,
					const boost::shared_ptr<Segments>& sopnetSegments,
					const boost::shared_ptr<SegmentTrees>& sopnetNeurons,
					const boost::shared_ptr<Segments>& blockSegments,
					const boost::shared_ptr<SegmentTrees>& blockNeurons)
{
	LOG_USER(out) << "For " << title << ": (Sopnet / Core)" << endl;
	LOG_USER(out) << "Segments:" << endl;
	LOG_USER(out) << "    Ends:          " << sopnetSegments->getEnds().size() << "\t" <<
		blockSegments->getEnds().size() << endl;
	LOG_USER(out) << "    Continuations: " << sopnetSegments->getContinuations().size() << "\t" <<
		blockSegments->getContinuations().size() << endl;
	LOG_USER(out) << "    Branches:      " << sopnetSegments->getBranches().size() << "\t" <<
		blockSegments->getBranches().size() << endl;
	LOG_USER(out) << "Neurons: " << sopnetNeurons->size() << "\t" << blockNeurons->size() << endl;
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
	
	writeSliceImages(sliceStore, blocks);
	
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
	
	blockSize50 = util::point3<unsigned int>(fractionCeiling(stackSize.x, 1, 2),
											 fractionCeiling(stackSize.y, 1, 2), stackSize.z);
	// Blocks of this size mean that the blocks don't fit exactly within stack boundaries.
	// The result should not change in this case.
	blockSize40 = util::point3<unsigned int>(fractionCeiling(stackSize.x, 2, 5),
											 fractionCeiling(stackSize.y, 2, 5), stackSize.z);
	
	segmentationCostParameters->weightPotts = 0;
	segmentationCostParameters->weight = 0;
	segmentationCostParameters->priorForeground = 0.2;
	
	priorCostFunctionParameters->priorContinuation = -100;
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
	
	compareResults("full stack", sopnetSegments, sopnetNeurons, coreSolverSegments, coreSolverNeurons);
	compareResults("half blocks", sopnetSegments, sopnetNeurons, coreSolverSegmentsBlock, coreSolverNeuronsBlock);
	compareResults("sequential half blocks", sopnetSegments, sopnetNeurons, coreSolverSegmentsSequential, coreSolverNeuronsSequential);
	compareResults("40% blocks", sopnetSegments, sopnetNeurons, coreSolverSegmentsOvlpBound, coreSolverNeuronsOvlpBound);
	compareResults("sequential 40% blocks", sopnetSegments, sopnetNeurons, coreSolverSegmentsOvlpBoundSequential, coreSolverNeuronsOvlpBoundSequential);

}

int main(int optionc, char** optionv)
{
	util::ProgramOptions::init(optionc, optionv);
	LogManager::init();
	
	LOG_DEBUG(out) << "what?" << std::endl;
	
	/*
	 * Assumes membrane path is ./membranes and raw image path is ./raw 
	 * Requires a file ./segment_rf.hdf compatible with these images
	 */
	
	try
	{
		cout << "go?" << endl;
		std::string membranePath = "./membranes";
		std::string rawPath = "./raw";
		pipeline::Value<ImageStack> testStack;
		unsigned int nx, ny, nz;
		util::point3<unsigned int> stackSize;
		util::point3<unsigned int>  blockSize50, blockSize40;
		
		boost::shared_ptr<ImageStackDirectoryReader> directoryStack =
			boost::make_shared<ImageStackDirectoryReader>(membranePath);
		
		testStack = directoryStack->getOutput();
		nx = testStack->width();
		ny = testStack->height();
		nz = testStack->size();
		
		stackSize = point3<unsigned int>(nx, ny, nz);
		
		blockSize50 = util::point3<unsigned int>(fractionCeiling(stackSize.x, 1, 2),
									fractionCeiling(stackSize.y, 1, 2), stackSize.z);
		// Blocks of this size mean that the blocks don't fit exactly within stack boundaries.
		// The result should not change in this case.
		blockSize40 = util::point3<unsigned int>(fractionCeiling(stackSize.x, 2, 5),
									fractionCeiling(stackSize.y, 2, 5), stackSize.z);
		
// 		writeAllSlices(testStack, yep);
		
		testStack->clear();
		
		
		testSliceGuarantor(membranePath, stackSize, blockSize40);
		
		testSolver(membranePath,
					 rawPath,
					 stackSize);
	}
	catch (Exception& e)
	{
		handleException(e);
	}
}
