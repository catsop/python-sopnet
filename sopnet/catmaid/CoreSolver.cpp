#include "CoreSolver.h"
#include <boost/make_shared.hpp>
#include <util/ProgramOptions.h>
#include <pipeline/Value.h>


util::ProgramOption optionRandomForestFileBlock(
		util::_module           = "blockSolver",
		util::_long_name        = "segmentRandomForest",
		util::_description_text = "Path to an HDF5 file containing the segment random forest.",
		util::_default_value    = "segment_rf.hdf");

CoreSolver::CoreSolver() :
	_problemAssembler(boost::make_shared<ProblemAssembler>()),
	//_componentTreeExtractor(boost::make_shared<ComponentTreeExtractor>()),	
	_reconstructor(boost::make_shared<Reconstructor>()),
	_linearSolver(boost::make_shared<LinearSolver>()),
	_neuronExtractor(boost::make_shared<NeuronExtractor>()),
	_segmentReader(boost::make_shared<SegmentReader>()),
	_sliceReader(boost::make_shared<SliceReader>()),
	_priorCostFunction(boost::make_shared<PriorCostFunction>()),
	_segmentationCostFunction(boost::make_shared<SegmentationCostFunction>()),
	_randomForestHDF5Reader(
		boost::make_shared<RandomForestHdf5Reader>(optionRandomForestFileBlock.as<std::string>())),
	_rawImageStackReader(boost::make_shared<ImageBlockStackReader>()),
	_membraneStackReader(boost::make_shared<ImageBlockStackReader>()),
	_randomForestCostFunction(boost::make_shared<RandomForestCostFunction>()),
	_segmentFeaturesExtractor(boost::make_shared<SegmentFeaturesExtractor>()),
	_objectiveGenerator(boost::make_shared<ObjectiveGenerator>())
{
	registerInput(_priorCostFunctionParameters, "prior cost parameters");
	registerInput(_blocks, "blocks");
	
	registerInput(_segmentationCostFunctionParameters, "segmentation cost parameters",
				pipeline::Optional);
	
	registerInput(_segmentStore, "segment store");
	registerInput(_sliceStore, "slice store");
	registerInput(_rawImageFactory, "raw image factory");
	registerInput(_membraneFactory, "membrane factory");
	registerInput(_forceExplanation, "force explanation");
	
	registerOutput(_neurons, "neurons");
	//registerOutput(_problemAssembler->getOutput("segments"), "segments");
}

void
CoreSolver::updateOutputs()
{
	boost::shared_ptr<LinearSolverParameters> binarySolverParameters = 
		boost::make_shared<LinearSolverParameters>(Binary);
	pipeline::Value<SegmentTrees> neurons;
		
	boost::shared_ptr<Blocks> boundingBlocks;
		
		
	_segmentReader->setInput("blocks", _blocks);
	_sliceReader->setInput("blocks", _blocks);
	
	_segmentReader->setInput("store", _segmentStore);
	_sliceReader->setInput("store", _sliceStore);
	
	_rawImageStackReader->setInput("factory", _rawImageFactory);
	
// 	_componentTreeExtractor->setInput("slices", _sliceReader->getOutput("slices"));
// 	_componentTreeExtractor->setInput("segments", _segmentReader->getOutput("segments"));
// 	_componentTreeExtractor->setInput("blocks", _blocks);
// 	_componentTreeExtractor->setInput("store", _sliceStore);
// 	_componentTreeExtractor->setInput("force explanation", _forceExplanation);
// 	
	_problemAssembler->addInput("segments", _segmentReader->getOutput("segments"));
// 	_problemAssembler->addInput("linear constraints",
// 								_componentTreeExtractor->getOutput("linear constraints"));
// 	
	// Use problem assembler output to compute the bounding box we need to contain all
	// of the slices that are present. This will usually be larger than the requested
	// Blocks.
	boundingBlocks = computeBound();
	pipeline::Value<util::point3<unsigned int> > offset(boundingBlocks->location());;
	_rawImageStackReader->setInput("block", boundingBlocks);
	
	
	
	_segmentFeaturesExtractor->setInput("crop offset", offset);
	_segmentFeaturesExtractor->setInput("segments", _problemAssembler->getOutput("segments"));
	_segmentFeaturesExtractor->setInput("raw sections", _rawImageStackReader->getOutput());
	
	_priorCostFunction->setInput("parameters", _priorCostFunctionParameters);
	
	_randomForestCostFunction->setInput("random forest",
										_randomForestHDF5Reader->getOutput("random forest"));
	_randomForestCostFunction->setInput("features",
										_segmentFeaturesExtractor->getOutput("all features"));
	
	
	_objectiveGenerator->setInput("segments", _problemAssembler->getOutput("segments"));
	_objectiveGenerator->setInput("segment cost function",
								  _randomForestCostFunction->getOutput("cost function"));
	_objectiveGenerator->addInput("additional cost functions",
								  _priorCostFunction->getOutput("cost function"));
	
	if (_segmentationCostFunctionParameters)
	{
		_membraneStackReader->setInput("factory", _membraneFactory);
		_membraneStackReader->setInput("block", boundingBlocks);
		_segmentationCostFunction->setInput("membranes", _membraneStackReader->getOutput());
		_segmentationCostFunction->setInput("parameters", _segmentationCostFunctionParameters);
		_segmentationCostFunction->setInput("crop offset", offset);
		_objectiveGenerator->addInput("additional cost functions",
									  _segmentationCostFunction->getOutput("cost function"));
	}

	
	_linearSolver->setInput("objective", _objectiveGenerator->getOutput());
	_linearSolver->setInput("linear constraints", _problemAssembler->getOutput("linear constraints"));
	_linearSolver->setInput("parameters", binarySolverParameters);
	
	_reconstructor->setInput("segments", _problemAssembler->getOutput("segments"));
	_reconstructor->setInput("solution", _linearSolver->getOutput());

	_neuronExtractor->setInput("segments", _reconstructor->getOutput());
	
	std::cout << " GO! " << std::endl;
	
	neurons = _neuronExtractor->getOutput();
	*_neurons = *neurons;
}

boost::shared_ptr<Blocks>
CoreSolver::computeBound()
{
	pipeline::Value<Segments> segments = _problemAssembler->getOutput("segments");
	if (segments->size() > 0)
	{
			
		util::rect<int> bound =
			segments->getSegments()[0]->getSlices()[0]->getComponent()->getBoundingBox();
		boost::shared_ptr<Box<> > box;
		boost::shared_ptr<Blocks> boundBlocks;
			
		foreach (boost::shared_ptr<Segment> segment, segments->getSegments())
		{
			foreach (boost::shared_ptr<Slice> slice, segment->getSlices())
			{
				util::rect<int> componentBound = slice->getComponent()->getBoundingBox();
				bound.fit(componentBound);
			}
		}
		
		box = boost::make_shared<Box<> >(bound, _blocks->location().z, _blocks->size().z);
		
		boundBlocks = _blocks->getManager()->blocksInBox(box);
		boundBlocks->expand(util::point3<int>(0,0,1));
		return boundBlocks;
	}
	else
	{
		return _blocks;
	}
}


