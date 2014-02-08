#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem.hpp>

#include <imageprocessing/ImageStack.h>
#include <util/Logger.h>
#include <util/point3.hpp>
#include <pipeline/all.h>
#include <pipeline/Value.h>
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
#include <catmaidsopnet/CoreSolver.h>
#include <catmaidsopnet/SegmentGuarantor.h>
#include <catmaidsopnet/SliceGuarantor.h>
#include <catmaidsopnet/persistence/LocalSegmentStore.h>
#include <catmaidsopnet/persistence/LocalSliceStore.h>
#include <sopnet/block/Box.h>
#include <vigra/impex.hxx>
#include <sopnet/slices/SliceExtractor.h>

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
		const boost::shared_ptr<pipeline::Wrap<bool> >& forceExplanation)
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
					  const boost::shared_ptr<BlockManager>& manager)
{
	std::string path = "./experiment_" + boost::lexical_cast<std::string>(experiment++);
	boost::filesystem::path dir(path);
	pipeline::Value<Slices> slices;
	boost::shared_ptr<SliceReader> reader = boost::make_shared<SliceReader>();
	
	boost::filesystem::create_directory(dir);
	
	reader->setInput("store", store);
	reader->setInput("block manager", manager);
	reader->setInput("box", manager->blocksInBox(
		boost::make_shared<Box<> >(util::ptrTo(0u,0u,0u), manager->stackSize())));
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
	sopnet->setInput("slices", membraneReader->getOutput());
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



void coreSolver(const std::string& membranePath, const std::string& rawPath,
	const boost::shared_ptr<SegmentationCostFunctionParameters>& segmentationCostParameters,
	const boost::shared_ptr<PriorCostFunctionParameters>& priorCostFunctionParameters,
	const boost::shared_ptr<pipeline::Wrap<bool> >& forceExplanation,
	const boost::shared_ptr<Segments>& segmentsOut,
	const boost::shared_ptr<SegmentTrees>& neuronsOut,
	
	const boost::shared_ptr<util::point3<unsigned int> >& stackSize,
	const boost::shared_ptr<util::point3<unsigned int> >& blockSize,
	const bool extractSimultaneousBlocks)
{

	boost::shared_ptr<BlockManager> blockManager =
		boost::make_shared<LocalBlockManager>(stackSize, blockSize);
	boost::shared_ptr<Box<> > box =
		boost::make_shared<Box<> >(util::ptrTo(0u, 0u, 0u), stackSize);
	boost::shared_ptr<pipeline::Wrap<unsigned int> > maxSize =
		boost::make_shared<pipeline::Wrap<unsigned int> >(1024 * 1024 * 64);
	boost::shared_ptr<Blocks> blocks = blockManager->blocksInBox(box);
	
	// Block pipeline variables
	boost::shared_ptr<ImageBlockFactory> membraneBlockFactory =
		boost::shared_ptr<ImageBlockFactory>(new ImageBlockFileFactory(membranePath));
	boost::shared_ptr<ImageBlockFactory> rawBlockFactory =
		boost::shared_ptr<ImageBlockFactory>(new ImageBlockFileFactory(rawPath));
	boost::shared_ptr<LocalSliceStore> localSliceStore =
		boost::shared_ptr<LocalSliceStore>(new LocalSliceStore());
	boost::shared_ptr<SliceStore> sliceStore = localSliceStore;
	boost::shared_ptr<SliceGuarantor> sliceGuarantor = boost::make_shared<SliceGuarantor>();
	boost::shared_ptr<SegmentGuarantor> segmentGuarantor =
		boost::make_shared<SegmentGuarantor>();
	boost::shared_ptr<SegmentStore> segmentStore =
		boost::shared_ptr<SegmentStore>(new LocalSegmentStore());
	boost::shared_ptr<SegmentReader> segmentReader = boost::make_shared<SegmentReader>();
	boost::shared_ptr<SliceReader> sliceReader= boost::make_shared<SliceReader>();
	boost::shared_ptr<CoreSolver> coreSolver = boost::make_shared<CoreSolver>();
	
	// Result Values
	pipeline::Value<SliceStoreResult> sliceResult;
	pipeline::Value<SegmentStoreResult> segmentResult;
	pipeline::Value<SegmentTrees> neurons;
	pipeline::Value<Segments> segments;
	pipeline::Value<Slices> slices;
	
	// pipeline


	sliceGuarantor->setInput("block manager", blockManager);
	sliceGuarantor->setInput("store", sliceStore);
	sliceGuarantor->setInput("block factory", membraneBlockFactory);
	sliceGuarantor->setInput("force explanation", forceExplanation);
	sliceGuarantor->setInput("maximum area", maxSize);

	segmentGuarantor->setInput("block manager", blockManager);
	segmentGuarantor->setInput("segment store", segmentStore);
	segmentGuarantor->setInput("slice store", sliceStore);
	segmentGuarantor->setInput("force explanation", forceExplanation);

	if (extractSimultaneousBlocks)
	{
		sliceGuarantor->setInput("box", blocks);
		segmentGuarantor->setInput("box", blocks);
		
		sliceResult = sliceGuarantor->getOutput();
		
		LOG_USER(out) << "Guaranteed " << sliceResult->count << " slices" << std::endl;
		
		segmentResult = segmentGuarantor->getOutput();
		
		LOG_USER(out) << "Guaranteed " << segmentResult->count << " segments" << std::endl;
	}
	else
	{
		foreach (const boost::shared_ptr<Block> block, *blocks)
		{
			sliceGuarantor->setInput("box", block);
			segmentGuarantor->setInput("box", block);

			sliceResult = sliceGuarantor->getOutput();

			LOG_USER(out) << "Guaranteed " << sliceResult->count << " slices" << std::endl;

			segmentResult = segmentGuarantor->getOutput();

			LOG_USER(out) << "Guaranteed " << segmentResult->count << " segments" << std::endl;
		}
	}
	
	LOG_USER(out) << "Setting up block solver" << std::endl;
	coreSolver->setInput("prior cost parameters",
							priorCostFunctionParameters);
	coreSolver->setInput("segmentation cost parameters", segmentationCostParameters);
	coreSolver->setInput("blocks", blocks);
	coreSolver->setInput("segment store", segmentStore);
	coreSolver->setInput("slice store", sliceStore);
	coreSolver->setInput("raw image factory", rawBlockFactory);
	coreSolver->setInput("membrane factory", membraneBlockFactory);
	coreSolver->setInput("force explanation", forceExplanation);
	
	neurons = coreSolver->getOutput("neurons");
	segments = coreSolver->getProblemAssembler()->getOutput("segments");
	
	LOG_USER(out) << "Solved " << neurons->size() << " neurons" << std::endl;
	
	segmentsOut->addAll(segments);
	neuronsOut->addAll(neurons);
	
	localSliceStore->dumpStore();
	
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
		cout << "go?" << endl;
		std::string membranePath = "./membranes";
		std::string rawPath = "./raw";
		pipeline::Value<ImageStack> testStack;
		unsigned int nx, ny, nz;
		boost::shared_ptr<util::point3<unsigned int> > stackSize, blockSize50, blockSize40;
		boost::shared_ptr<SegmentationCostFunctionParameters> segmentationCostParameters = 
			boost::make_shared<SegmentationCostFunctionParameters>();
		boost::shared_ptr<PriorCostFunctionParameters> priorCostFunctionParameters = 
			boost::make_shared<PriorCostFunctionParameters>();
		boost::shared_ptr<pipeline::Wrap<bool> > yep =
			boost::make_shared<pipeline::Wrap<bool> >(true);
		
		boost::shared_ptr<ImageStackDirectoryReader> directoryStack =
			boost::make_shared<ImageStackDirectoryReader>(membranePath);
			
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
		
		testStack = directoryStack->getOutput();
		nx = testStack->width();
		ny = testStack->height();
		nz = testStack->size();
		
// 		writeAllSlices(testStack, yep);
		
		testStack->clear();
		
		stackSize = util::ptrTo(nx, ny, nz);
		blockSize50 = util::ptrTo(fractionCeiling(nx, 1, 2), fractionCeiling(ny, 1, 2), nz);
		// Blocks of this size mean that the blocks don't fit exactly within stack boundaries.
		// The result should not change in this case.
		blockSize40 = util::ptrTo(fractionCeiling(nx, 2, 5), fractionCeiling(ny, 2, 5), nz);
		
		segmentationCostParameters->weightPotts = 0;
		segmentationCostParameters->weight = 0;
		segmentationCostParameters->priorForeground = 0.2;
		
		priorCostFunctionParameters->priorContinuation = -100;
		priorCostFunctionParameters->priorBranch = -100;
		
		LOG_USER(out) << "Stack size: " << *stackSize << endl;
		
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
	catch (Exception& e)
	{
		handleException(e);
	}
}
