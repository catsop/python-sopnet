
#include <iostream>
#include <string>
#include <fstream>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include <boost/progress.hpp>

#include <pipeline/all.h>
#include <pipeline/Value.h>
#include <util/exceptions.h>
#include <imageprocessing/gui/ImageView.h>
#include <imageprocessing/gui/ImageStackView.h>
#include <imageprocessing/io/ImageHttpReader.h>
#include <imageprocessing/io/ImageFileReader.h>
#include <imageprocessing/io/ImageBlockStackReader.h>
#include <util/ProgramOptions.h>
#include <ImageMagick/Magick++.h>
#include <sopnet/sopnet/block/Block.h>
#include <sopnet/sopnet/block/Blocks.h>
#include <sopnet/sopnet/block/Box.h>
#include <sopnet/sopnet/block/BlockManager.h>
#include <sopnet/sopnet/block/LocalBlockManager.h>
#include <sopnet/inference/SegmentationCostFunctionParameters.h>

#include <imageprocessing/io/ImageBlockFileReader.h>
#include <util/point3.hpp>
#include <catmaidsopnet/SliceGuarantorParameters.h>
#include <catmaidsopnet/SliceGuarantor.h>
#include <catmaidsopnet/SegmentGuarantor.h>
#include <catmaidsopnet/persistence/SliceStore.h>
#include <catmaidsopnet/persistence/LocalSliceStore.h>
#include <catmaidsopnet/persistence/LocalSegmentStore.h>
#include <catmaidsopnet/persistence/SliceReader.h>
#include <catmaidsopnet/persistence/SegmentReader.h>
#include <catmaidsopnet/ConsistencyConstraintExtractor.h>
#include <catmaidsopnet/BlockSolver.h>

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

void testBlockPipeline()
{
	unsigned int id = 1;
	
	
	
    try
    {
		

		LOG_USER(out) << "Start" << endl;
		
		boost::unordered_set<Slice> sliceSet = boost::unordered_set<Slice>();
		
		std::string seriesDirectory = "/nfs/data0/home/larry/code/sopnet/data/testmembrane";
		std::string rawDirectory = "/nfs/data0/home/larry/code/sopnet/data/raw";
		boost::shared_ptr<BlockManager> blockManager = boost::make_shared<LocalBlockManager>(util::ptrTo(1024u, 1024u, 20u), util::ptrTo(256u, 256u, 5u));
		boost::shared_ptr<Box<> > box = boost::make_shared<Box<> >(util::ptrTo(256u, 768u, 0u), util::ptrTo(512u, 256u, 10u));
		boost::shared_ptr<pipeline::Wrap<unsigned int> > maxSize = boost::make_shared<pipeline::Wrap<unsigned int> >(256 * 256 * 64);
		
		LOG_USER(out) << "Initting pipeline data" << endl;
		boost::shared_ptr<ImageBlockFactory> imageBlockFactory = boost::shared_ptr<ImageBlockFactory>(new ImageBlockFileFactory(seriesDirectory));
		boost::shared_ptr<ImageBlockFactory> rawBlockFactory = boost::shared_ptr<ImageBlockFactory>(new ImageBlockFileFactory(rawDirectory));
		boost::shared_ptr<pipeline::Wrap<bool> > nope = boost::make_shared<pipeline::Wrap<bool> >(true);
		boost::shared_ptr<SliceStore> sliceStore = boost::shared_ptr<SliceStore>(new LocalSliceStore());
		boost::shared_ptr<SliceGuarantor> sliceGuarantor = boost::make_shared<SliceGuarantor>();
		boost::shared_ptr<SegmentGuarantor> segmentGuarantor = boost::make_shared<SegmentGuarantor>();
		boost::shared_ptr<SegmentStore> segmentStore = boost::shared_ptr<SegmentStore>(new LocalSegmentStore());
		boost::shared_ptr<SegmentReader> segmentReader = boost::make_shared<SegmentReader>();
		boost::shared_ptr<SliceReader> sliceReader= boost::make_shared<SliceReader>();
		boost::shared_ptr<ConsistencyConstraintExtractor> constraintExtractor = 
			boost::make_shared<ConsistencyConstraintExtractor>();
		boost::shared_ptr<Blocks> blocks = blockManager->blocksInBox(box);
		LOG_USER(out) << "initting block solver" << std::endl;
		boost::shared_ptr<BlockSolver> blockSolver = boost::make_shared<BlockSolver>();
		boost::shared_ptr<SegmentationCostFunctionParameters> segmentationCostParameters = 
			boost::make_shared<SegmentationCostFunctionParameters>();
		boost::shared_ptr<PriorCostFunctionParameters> priorCostFunctionParameters = 
			boost::make_shared<PriorCostFunctionParameters>();
		
		pipeline::Value<SliceStoreResult> sliceResult;
		pipeline::Value<SegmentStoreResult> segmentResult;
		pipeline::Value<Slices> slices;
		pipeline::Value<Segments> segments;
		pipeline::Value<LinearConstraints> constraints;
		pipeline::Value<SegmentTrees> neurons;
		
		sliceGuarantor->setInput("box", box);
		sliceGuarantor->setInput("store", sliceStore);
		sliceGuarantor->setInput("block manager", blockManager);
		sliceGuarantor->setInput("block factory", imageBlockFactory);
		sliceGuarantor->setInput("force explanation", nope);
		sliceGuarantor->setInput("maximum area", maxSize);

		LOG_USER(out) << "Guaranteeing some slices." << endl;
		sliceResult = sliceGuarantor->getOutput();
		LOG_USER(out) << "Guaranteed " << sliceResult->count << " slices." << endl;
		
		
		LOG_USER(out) << "Let's read them back out" << endl;

		
		sliceReader->setInput("block manager", blockManager);
		sliceReader->setInput("store", sliceStore);
		sliceReader->setInput("box", box);
		
		LOG_USER(out) << "Attempting to read slices" << endl;
		
		slices = sliceReader->getOutput();

		LOG_USER(out) << "Read " << slices->size() << " slices" << endl;
		
		unsigned int minZ = (*slices)[0]->getSection();
		unsigned int maxZ = minZ;
		
		foreach (boost::shared_ptr<Slice> zSlice, *slices)
		{
			unsigned int section = zSlice->getSection();
			minZ = minZ > section ? section : minZ;
			maxZ = maxZ < section ? section : maxZ;
		}
		
		LOG_USER(out) << "Requested box with minZ " << box->location().z << " and z-depth " << box->size().z
			<< ", got slices from " << minZ << " to " << maxZ << std::endl;
		
		segmentGuarantor->setInput("box", box);
		segmentGuarantor->setInput("segment store", segmentStore);
		segmentGuarantor->setInput("slice store", sliceStore);
		segmentGuarantor->setInput("block manager", blockManager);
		
		segmentResult = segmentGuarantor->getOutput();
		
		LOG_USER(out) << "Guaranteed " << segmentResult->count << " segments" << std::endl;
		
		segmentReader->setInput("box", box);
		segmentReader->setInput("block manager", blockManager);
		segmentReader->setInput("store", segmentStore);
		
		segments = segmentReader->getOutput();
		
		LOG_USER(out) << "Read back " << segments->size() << " segments" << std::endl;
		
		constraintExtractor->setInput("slices", sliceReader->getOutput());
		constraintExtractor->setInput("segments", segmentReader->getOutput());
		
		constraints = constraintExtractor->getOutput();
		
		LOG_USER(out) << "Extracted " << constraints->size() << " constraints" << std::endl;
		
		segmentationCostParameters->weightPotts = 0;
		segmentationCostParameters->weight = 0;
		segmentationCostParameters->priorForeground = 0.2;
		
		priorCostFunctionParameters->priorContinuation = -100;
		priorCostFunctionParameters->priorBranch = -100;
		
		LOG_USER(out) << "Setting up block solver" << std::endl;
		blockSolver->setInput("prior cost parameters",
							  priorCostFunctionParameters);
		blockSolver->setInput("segmentation cost parameters", segmentationCostParameters);
		LOG_USER(out) << "blocks" << std::endl;
		blockSolver->setInput("blocks", blocks);
		LOG_USER(out) << "segment store" << std::endl;
		blockSolver->setInput("segment store", segmentStore);
		LOG_USER(out) << "slice store" << std::endl;
		blockSolver->setInput("slice store", sliceStore);
		LOG_USER(out) << "raw factory" << std::endl;
		blockSolver->setInput("raw image factory", rawBlockFactory);
		LOG_USER(out) << "membrane factory" << std::endl;
		blockSolver->setInput("membrane factory", imageBlockFactory);
		LOG_USER(out) << "Attempting to get output" << std::endl;
		neurons = blockSolver->getOutput();
		
		LOG_USER(out) << "Solved " << neurons->size() << " neurons" << std::endl;

	}
    catch (Exception& e)
    {
		handleException(e);
	}
}


int main(int optionc, char** optionv)
{
	util::ProgramOptions::init(optionc, optionv);
	LogManager::init();
	
	try
	{
		std::string seriesDirectory = "/nfs/data0/home/larry/code/sopnet/data/testmembrane";
		std::string rawDirectory = "/nfs/data0/home/larry/code/sopnet/data/raw";
		
		// Block management variables
		boost::shared_ptr<BlockManager> blockManager =
			boost::make_shared<LocalBlockManager>(util::ptrTo(1024u, 1024u, 20u),
												util::ptrTo(256u, 256u, 5u));
		boost::shared_ptr<Box<> > box =
			boost::make_shared<Box<> >(util::ptrTo(256u, 768u, 0u), util::ptrTo(512u, 256u, 10u));
		boost::shared_ptr<pipeline::Wrap<unsigned int> > maxSize =
			boost::make_shared<pipeline::Wrap<unsigned int> >(256 * 256 * 64);
		
		// Block pipeline variables
		boost::shared_ptr<ImageBlockFactory> imageBlockFactory =
			boost::shared_ptr<ImageBlockFactory>(new ImageBlockFileFactory(seriesDirectory));
		boost::shared_ptr<ImageBlockFactory> rawBlockFactory =
			boost::shared_ptr<ImageBlockFactory>(new ImageBlockFileFactory(rawDirectory));
		boost::shared_ptr<pipeline::Wrap<bool> > yep =
			boost::make_shared<pipeline::Wrap<bool> >(true);
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
		
		// Solver parameters
		boost::shared_ptr<SegmentationCostFunctionParameters> segmentationCostParameters = 
			boost::make_shared<SegmentationCostFunctionParameters>();
		boost::shared_ptr<PriorCostFunctionParameters> priorCostFunctionParameters = 
			boost::make_shared<PriorCostFunctionParameters>();
		
		// Result Values
		pipeline::Value<SliceStoreResult> sliceResult;
		pipeline::Value<SegmentStoreResult> segmentResult;
		
		// pipeline
		
		sliceGuarantor->setInput("box", box);
		sliceGuarantor->setInput("store", sliceStore);
		sliceGuarantor->setInput("block manager", blockManager);
		sliceGuarantor->setInput("block factory", imageBlockFactory);
		sliceGuarantor->setInput("force explanation", yep);
		sliceGuarantor->setInput("maximum area", maxSize);
		
		segmentGuarantor->setInput("box", box);
		segmentGuarantor->setInput("segment store", segmentStore);
		segmentGuarantor->setInput("slice store", sliceStore);
		segmentGuarantor->setInput("block manager", blockManager);
		
		segmentationCostParameters->weightPotts = 0;
		segmentationCostParameters->weight = 0;
		segmentationCostParameters->priorForeground = 0.2;
		
		priorCostFunctionParameters->priorContinuation = -100;
		priorCostFunctionParameters->priorBranch = -100;
		
		sliceResult = sliceGuarantor->getOutput();
		
		LOG_USER(out) << "Guaranteed " << sliceResult->count << " slices" << std::endl;
		
		segmentResult = segmentGuarantor->getOutput();
		
		LOG_USER(out) << "Guaranteed " << segmentResult->count << " segments" << std::endl;
		
		
	}
	catch (Exception& e)
	{
		handleException(e);
	}
	
	
	
}
