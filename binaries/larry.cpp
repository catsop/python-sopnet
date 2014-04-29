
#include <iostream>
#include <string>
#include <fstream>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include <boost/progress.hpp>

#include <imageprocessing/ImageStack.h>

#include <sopnet/Sopnet.h>

#include <pipeline/all.h>
#include <pipeline/Value.h>
#include <util/exceptions.h>
#include <imageprocessing/gui/ImageView.h>
#include <imageprocessing/gui/ImageStackView.h>
#include <imageprocessing/io/ImageHttpReader.h>
#include <imageprocessing/io/ImageFileReader.h>
#include <imageprocessing/io/ImageBlockStackReader.h>
#include <imageprocessing/io/ImageStackDirectoryReader.h>
#include <util/ProgramOptions.h>
#include <ImageMagick/Magick++.h>
#include <sopnet/sopnet/block/Block.h>
#include <sopnet/sopnet/block/Blocks.h>
#include <sopnet/sopnet/block/Box.h>
#include <sopnet/sopnet/block/CoreManager.h>
#include <sopnet/sopnet/block/LocalCoreManager.h>
#include <sopnet/inference/SegmentationCostFunctionParameters.h>

#include <imageprocessing/io/ImageBlockFileReader.h>
#include <util/point3.hpp>
#include <catmaid/SliceGuarantorParameters.h>
#include <catmaid/SliceGuarantor.h>
#include <catmaid/SegmentGuarantor.h>
#include <catmaid/persistence/SliceStore.h>
#include <catmaid/persistence/LocalSliceStore.h>
#include <catmaid/persistence/LocalSegmentStore.h>
#include <catmaid/persistence/SliceReader.h>
#include <catmaid/persistence/SegmentReader.h>
#include <catmaid/ConsistencyConstraintExtractor.h>
#include <catmaid/CoreSolver.h>

#include <boost/unordered_set.hpp>

using util::point3;
using std::cout;
using std::endl;
using namespace gui;
using namespace logger;

void handleException(boost::exception& e) {

	LOG_ERROR(out) << "[window thread] caught exception: ";

	if (boost::get_error_info<error_message>(e))
		LOG_ERROR(out) << *boost::get_error_info<error_message>(e);

	if (boost::get_error_info<stack_trace>(e))
		LOG_ERROR(out) << *boost::get_error_info<stack_trace>(e);

	LOG_ERROR(out) << std::endl;

	LOG_ERROR(out) << "[window thread] details: " << std::endl
	               << boost::diagnostic_information(e)
	               << std::endl;

	exit(-1);
}

pipeline::Value<SegmentTrees>
sopnetSolver(const std::string& membranePath, const std::string& rawPath,
	const boost::shared_ptr<SegmentationCostFunctionParameters>& segmentationCostParameters,
	const boost::shared_ptr<PriorCostFunctionParameters>& priorCostFunctionParameters,
	const boost::shared_ptr<pipeline::Wrap<bool> >& forceExplanation,
	const boost::shared_ptr<Segments>& segmentsOut)
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
	
	return neurons;
}

pipeline::Value<SegmentTrees>
blockSolver(const std::string& membranePath, const std::string& rawPath,
	const boost::shared_ptr<SegmentationCostFunctionParameters>& segmentationCostParameters,
	const boost::shared_ptr<PriorCostFunctionParameters>& priorCostFunctionParameters,
	const boost::shared_ptr<pipeline::Wrap<bool> >& forceExplanation,
	const boost::shared_ptr<Segments>& segmentsOut)
{
		// Block management variables
	boost::shared_ptr<util::point3<unsigned int> > stackSize = util::ptrTo(179u, 168u, 5u);
	boost::shared_ptr<util::point3<unsigned int> > blockSize = util::ptrTo(179u, 84u, 5u);
	boost::shared_ptr<CoreManager> blockManager =
		boost::make_shared<LocalCoreManager>(stackSize, blockSize);
	boost::shared_ptr<Box<> > box =
		boost::make_shared<Box<> >(util::ptrTo(0u, 0u, 0u), stackSize);
	pipeline::Value<unsigned int> maxSize(1024 * 1024 * 64);
	boost::shared_ptr<Blocks> blocks = blockManager->blocksInBox(box);
	
	// Block pipeline variables
	boost::shared_ptr<ImageBlockFactory> membraneBlockFactory =
		boost::shared_ptr<ImageBlockFactory>(new ImageBlockFileFactory(membranePath));
	boost::shared_ptr<ImageBlockFactory> rawBlockFactory =
		boost::shared_ptr<ImageBlockFactory>(new ImageBlockFileFactory(rawPath));
	boost::shared_ptr<SliceStore> sliceStore =
		boost::shared_ptr<SliceStore>(new LocalSliceStore());
	boost::shared_ptr<SliceGuarantor> sliceGuarantor = boost::make_shared<SliceGuarantor>();
	boost::shared_ptr<SegmentGuarantor> segmentGuarantor =
		boost::make_shared<SegmentGuarantor>();
	boost::shared_ptr<SegmentStore> segmentStore =
		boost::shared_ptr<SegmentStore>(new LocalSegmentStore());
	boost::shared_ptr<SegmentReader> segmentReader = boost::make_shared<SegmentReader>();
	boost::shared_ptr<SliceReader> sliceReader= boost::make_shared<SliceReader>();
	boost::shared_ptr<BlockSolver> blockSolver = boost::make_shared<BlockSolver>();
	
	// Result Values
	pipeline::Value<SliceStoreResult> sliceResult;
	pipeline::Value<SegmentStoreResult> segmentResult;
	pipeline::Value<SegmentTrees> neurons;
	pipeline::Value<Segments> segments;
	
	// pipeline
	
	sliceGuarantor->setInput("box", blocks);
	sliceGuarantor->setInput("block manager", blockManager);
	sliceGuarantor->setInput("store", sliceStore);
	sliceGuarantor->setInput("block factory", membraneBlockFactory);
	sliceGuarantor->setInput("force explanation", forceExplanation);
	sliceGuarantor->setInput("maximum area", maxSize);
	
	segmentGuarantor->setInput("box", blocks);
	segmentGuarantor->setInput("block manager", blockManager);
	segmentGuarantor->setInput("segment store", segmentStore);
	segmentGuarantor->setInput("slice store", sliceStore);
	
	sliceResult = sliceGuarantor->getOutput();
	
	LOG_USER(out) << "Guaranteed " << sliceResult->count << " slices" << std::endl;
	
	segmentResult = segmentGuarantor->getOutput();
	
	LOG_USER(out) << "Guaranteed " << segmentResult->count << " segments" << std::endl;
	
	
	LOG_USER(out) << "Setting up block solver" << std::endl;
	blockSolver->setInput("prior cost parameters",
							priorCostFunctionParameters);
	blockSolver->setInput("segmentation cost parameters", segmentationCostParameters);
	blockSolver->setInput("blocks", blocks);
	blockSolver->setInput("segment store", segmentStore);
	blockSolver->setInput("slice store", sliceStore);
	blockSolver->setInput("raw image factory", rawBlockFactory);
	blockSolver->setInput("membrane factory", membraneBlockFactory);
	blockSolver->setInput("force explanation", forceExplanation);
	
	neurons = blockSolver->getOutput("neurons");
	segments = blockSolver->getProblemAssembler()->getOutput("segments");
	
	LOG_USER(out) << "Solved " << neurons->size() << " neurons" << std::endl;
	
	segmentsOut->addAll(segments);
	
	return neurons;
}

void segmentNotFoundOutput(const boost::shared_ptr<EndSegment>& end)
{
	LOG_USER(out) << "Missing segment: " << (end->getDirection() == Left ? "left: " : "right: ");
	
	LOG_USER(out) << end->getSlice()->getSection() << " ";
	
	LOG_USER(out) << end->getSlice()->getComponent()->getBoundingBox() << std::endl;
}

int main(int optionc, char** optionv)
{
	util::ProgramOptions::init(optionc, optionv);
	LogManager::init();
	
	try
	{
		SegmentSet segmentSet;
		boost::shared_ptr<SegmentationCostFunctionParameters> segmentationCostParameters = 
			boost::make_shared<SegmentationCostFunctionParameters>();
		boost::shared_ptr<PriorCostFunctionParameters> priorCostFunctionParameters = 
			boost::make_shared<PriorCostFunctionParameters>();
		pipeline::Value<bool> yep(true);
		pipeline::Value<SegmentTrees> blockNeurons, sopnetNeurons;
		boost::shared_ptr<Segments> sopnetSegments = boost::make_shared<Segments>();
		boost::shared_ptr<Segments> blockSolverSegments = boost::make_shared<Segments>();
			
		std::string membranePath = "/nfs/data0/home/larry/code/sopnet/data/membranes";
		std::string rawPath = "/nfs/data0/home/larry/code/sopnet/data/raw";
		
		segmentationCostParameters->weightPotts = 0;
		segmentationCostParameters->weight = 0;
		segmentationCostParameters->priorForeground = 0.2;
		
		priorCostFunctionParameters->priorContinuation = -100;
		priorCostFunctionParameters->priorBranch = -100;

		LOG_USER(out) << "BLOCK SOLVE!!!!" << std::endl;
		blockNeurons = blockSolver(membranePath, rawPath, segmentationCostParameters,
								   priorCostFunctionParameters, yep, blockSolverSegments);
		LOG_USER(out) << "SOPNET SOLVE!!!!" << std::endl;
		sopnetNeurons = sopnetSolver(membranePath, rawPath, segmentationCostParameters,
									 priorCostFunctionParameters, yep, sopnetSegments);
		LOG_USER(out) << "Block solver produced " << blockNeurons->size() << " neurons." << std::endl;
		LOG_USER(out) << "Sopnet solver produced " << sopnetNeurons->size() << " neurons." << std::endl;

// 		foreach (boost::shared_ptr<EndSegment> end, blockSolverSegments->getEnds())
// 		{
// 			segmentSet.insert(end);
// 		}
		
// 		foreach (boost::shared_ptr<EndSegment> end, sopnetSegments->getEnds())
// 		{
// 			if (segmentSet.find(end) == segmentSet.end())
// 			{
// 				segmentNotFoundOutput(end);
// 			}
// 		}
		
		
	}
	catch (Exception& e)
	{
		handleException(e);
	}
}
