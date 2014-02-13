#include <boost/make_shared.hpp>

#include <catmaid/persistence/SegmentReader.h>
#include <catmaid/persistence/SliceReader.h>
#include <features/SegmentFeaturesExtractor.h>
#include <inference/io/RandomForestHdf5Reader.h>
#include <inference/LinearConstraint.h>
#include <inference/LinearConstraints.h>
#include <inference/LinearSolver.h>
#include <inference/LinearSolverParameters.h>
#include <inference/ObjectiveGenerator.h>
#include <inference/PriorCostFunction.h>
#include <inference/PriorCostFunctionParameters.h>
#include <inference/RandomForestCostFunction.h>
#include <inference/Reconstructor.h>
#include <inference/SegmentationCostFunction.h>
#include <neurons/NeuronExtractor.h>
#include <pipeline/Value.h>
#include <util/foreach.h>
#include <util/Logger.h>
#include <util/ProgramOptions.h>


#include "CoreSolver.h"

using std::vector;
using std::map;

logger::LogChannel coresolverlog("coresolverlog", "[CoreSolver] ");

util::ProgramOption optionRandomForestFileBlock(
		util::_module           = "blockSolver",
		util::_long_name        = "segmentRandomForest",
		util::_description_text = "Path to an HDF5 file containing the segment random forest.",
		util::_default_value    = "segment_rf.hdf");

CoreSolver::CoreSolver()
{
	registerInput(_priorCostFunctionParameters, "prior cost parameters");
	registerInput(_blocks, "blocks");
	
	registerInput(_segmentationCostFunctionParameters, "segmentation cost parameters",
				pipeline::Optional);
	
	registerInput(_segmentStore, "segment store");
	registerInput(_sliceStore, "slice store");
	registerInput(_rawImageStore, "raw image store");
	registerInput(_membraneStore, "membrane image store");
	registerInput(_forceExplanation, "force explanation");
	
	registerOutput(_neurons, "neurons");
	registerOutput(_outputSegments, "segments");
}

CoreSolver::ConstraintAssembler::ConstraintAssembler()
{
	registerInput(_segments, "segments");
	registerInput(_conflictSets, "conflict sets");
	registerInput(_assemblerForceExplanation, "force explanation");
	registerOutput(_constraints, "linear constraints");
}


void
CoreSolver::ConstraintAssembler::assembleConstraint(const ConflictSet& conflictSet,
									 map<unsigned int, vector<unsigned int> >& sliceSegmentsMap)
{

	boost::shared_ptr<LinearConstraint> constraint = boost::make_shared<LinearConstraint>();

	// for each slice in the constraint
	typedef map<unsigned int, double>::value_type pair_t;
	foreach (unsigned int sliceId, conflictSet.getSlices())
	{
		// for all the segments that involve this slice
		const vector<unsigned int> segmentIds = sliceSegmentsMap[sliceId];

		foreach (unsigned int segmentId, segmentIds)
		{
			constraint->setCoefficient(segmentId, 1.0);
		}
	}

	if (*_assemblerForceExplanation)
	{
		constraint->setRelation(Equal);
	}
	else
	{
		constraint->setRelation(LessEqual);
	}

	constraint->setValue(1);

	LOG_ALL(coresolverlog) << "created constraint " << *constraint << std::endl;
}

void CoreSolver::ConstraintAssembler::updateOutputs()
{
	map<unsigned int, vector<unsigned int> > sliceSegmentMap;
	
	foreach (boost::shared_ptr<Segment> segment, _segments->getSegments())
	{
		foreach (boost::shared_ptr<Slice> slice, segment->getSlices())
		{
			sliceSegmentMap[slice->getId()].push_back(segment->getId());
		}
	}
	
	foreach (const ConflictSet conflictSet, *_conflictSets)
	{
		assembleConstraint(conflictSet, sliceSegmentMap);
	}
}


void
CoreSolver::updateOutputs()
{
	// A whole mess of pipeline variables
	boost::shared_ptr<SliceReader> sliceReader = boost::make_shared<SliceReader>();
	boost::shared_ptr<SegmentReader> segmentReader = boost::make_shared<SegmentReader>();
	boost::shared_ptr<ProblemAssembler> problemAssembler = boost::make_shared<ProblemAssembler>();
	boost::shared_ptr<SegmentFeaturesExtractor> segmentFeaturesExtractor =
		boost::make_shared<SegmentFeaturesExtractor>();
	boost::shared_ptr<PriorCostFunction> priorCostFunction =
		boost::make_shared<PriorCostFunction>();
	boost::shared_ptr<RandomForestCostFunction> randomForestCostFunction = 
		boost::make_shared<RandomForestCostFunction>();
	boost::shared_ptr<ObjectiveGenerator> objectiveGenerator =
		boost::make_shared<ObjectiveGenerator>();
	boost::shared_ptr<SegmentationCostFunction> segmentationCostFunction =
		boost::make_shared<SegmentationCostFunction>();
	boost::shared_ptr<LinearSolver> linearSolver = boost::make_shared<LinearSolver>();
	boost::shared_ptr<Reconstructor> reconstructor = boost::make_shared<Reconstructor>();
	boost::shared_ptr<NeuronExtractor> neuronExtractor = boost::make_shared<NeuronExtractor>();
	boost::shared_ptr<ConstraintAssembler> constraintAssembler =
		boost::make_shared<ConstraintAssembler>();
	boost::shared_ptr<RandomForestHdf5Reader> randomForestHDF5Reader =
		boost::make_shared<RandomForestHdf5Reader>(optionRandomForestFileBlock.as<std::string>());

	// inputs and outputs
	boost::shared_ptr<LinearSolverParameters> binarySolverParameters = 
		boost::make_shared<LinearSolverParameters>(Binary);
	pipeline::Value<SegmentTrees> neurons;
	boost::shared_ptr<Blocks> boundingBlocks;
	pipeline::Value<Segments> segments;


	segmentReader->setInput("blocks", _blocks);
	sliceReader->setInput("blocks", _blocks);
	
	segmentReader->setInput("store", _segmentStore);
	sliceReader->setInput("store", _sliceStore);
	
	constraintAssembler->setInput("segments", segmentReader->getOutput("segments"));
	constraintAssembler->setInput("conflict sets", sliceReader->getOutput("conflict sets"));
	constraintAssembler->setInput("force explanation", _forceExplanation);
	
	problemAssembler->addInput("segments", segmentReader->getOutput("segments"));
	problemAssembler->addInput("linear constraints", constraintAssembler->getOutput("linear constraints"));
	

	// Use problem assembler output to compute the bounding box we need to contain all
	// of the slices that are present. This will usually be larger than the requested
	// Blocks.
	boundingBlocks = computeBound(problemAssembler);
	pipeline::Value<util::point3<unsigned int> > offset(boundingBlocks->location());;
	
	segmentFeaturesExtractor->setInput("crop offset", offset);
	segmentFeaturesExtractor->setInput("segments", problemAssembler->getOutput("segments"));
	segmentFeaturesExtractor->setInput("raw sections", _rawImageStore->getImageStack(*boundingBlocks));
	
	priorCostFunction->setInput("parameters", _priorCostFunctionParameters);
	
	randomForestCostFunction->setInput("random forest",
										randomForestHDF5Reader->getOutput("random forest"));
	randomForestCostFunction->setInput("features",
										segmentFeaturesExtractor->getOutput("all features"));
	
	
	objectiveGenerator->setInput("segments", problemAssembler->getOutput("segments"));
	objectiveGenerator->setInput("segment cost function",
								  randomForestCostFunction->getOutput("cost function"));
	objectiveGenerator->addInput("additional cost functions",
								  priorCostFunction->getOutput("cost function"));
	
	if (_segmentationCostFunctionParameters)
	{
		segmentationCostFunction->setInput("membranes", _membraneStore->getImageStack(*boundingBlocks));
		segmentationCostFunction->setInput("parameters", _segmentationCostFunctionParameters);
		segmentationCostFunction->setInput("crop offset", offset);
		objectiveGenerator->addInput("additional cost functions",
									  segmentationCostFunction->getOutput("cost function"));
	}

	
	linearSolver->setInput("objective", objectiveGenerator->getOutput());
	linearSolver->setInput("linear constraints", problemAssembler->getOutput("linear constraints"));
	linearSolver->setInput("parameters", binarySolverParameters);
	
	reconstructor->setInput("segments", problemAssembler->getOutput("segments"));
	reconstructor->setInput("solution", linearSolver->getOutput());

	neuronExtractor->setInput("segments", reconstructor->getOutput());
	
	std::cout << " GO! " << std::endl;
	
	neurons = neuronExtractor->getOutput();
	segments = problemAssembler->getOutput("segments");
	
	*_outputSegments = *segments;
	*_neurons = *neurons;
}

boost::shared_ptr<Blocks>
CoreSolver::computeBound(const boost::shared_ptr<ProblemAssembler> problemAssembler)
{
	pipeline::Value<Segments> segments = problemAssembler->getOutput("segments");
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


