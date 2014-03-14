#include <boost/make_shared.hpp>


#include <catmaid/persistence/SegmentReader.h>
#include <catmaid/persistence/SliceReader.h>
#include <features/SegmentFeaturesExtractor.h>
#include <inference/LinearConstraint.h>
#include <inference/LinearConstraints.h>
#include <inference/LinearCostFunction.h>
#include <inference/LinearSolver.h>
#include <inference/LinearSolverParameters.h>
#include <inference/ObjectiveGenerator.h>
#include <inference/PriorCostFunction.h>
#include <inference/PriorCostFunctionParameters.h>
#include <inference/Reconstructor.h>
#include <inference/SegmentationCostFunction.h>
#include <io/FileContentProvider.h>
#include <neurons/NeuronExtractor.h>
#include <pipeline/Value.h>
#include <sopnet/segments/SegmentSet.h>
#include <util/foreach.h>
#include <util/Logger.h>
#include <util/ProgramOptions.h>

#include <inference/io/LinearCostFunctionParametersReader.h>


#include "CoreSolver.h"

using std::vector;
using std::map;

logger::LogChannel coresolverlog("coresolverlog", "[CoreSolver] ");

util::ProgramOption optionLinearCostFunctionParametersFileBlock(
		util::_module           = "blockSolver",
		util::_long_name        = "linearCostFunctionParameters",
		util::_description_text = "Path to a file containing the weights for the linear cost function.",
		util::_default_value    = "./feature_weights.dat");

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


boost::shared_ptr<LinearConstraint>
CoreSolver::ConstraintAssembler::assembleConstraint(const ConflictSet& conflictSet,
									 map<unsigned int, vector<unsigned int> >& sliceSegmentsMap)
{

	boost::shared_ptr<LinearConstraint> constraint = boost::make_shared<LinearConstraint>();

	// for each slice in the constraint
	//typedef map<unsigned int, double>::value_type pair_t;
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
	
	return constraint;
}

void CoreSolver::ConstraintAssembler::updateOutputs()
{
	map<unsigned int, vector<unsigned int> > sliceSegmentMap;
	boost::shared_ptr<LinearConstraints> constraints = boost::make_shared<LinearConstraints>();
	
	foreach (boost::shared_ptr<Segment> segment, _segments->getSegments())
	{
		if (segment->getDirection() == Right)
		{
			foreach (boost::shared_ptr<Slice> slice, segment->getSourceSlices())
			{
				sliceSegmentMap[slice->getId()].push_back(segment->getId());
			}
		}
		else
		{
			foreach (boost::shared_ptr<Slice> slice, segment->getTargetSlices())
			{
				sliceSegmentMap[slice->getId()].push_back(segment->getId());
			}
		}
	}
	
	foreach (const ConflictSet conflictSet, *_conflictSets)
	{
		constraints->add(*assembleConstraint(conflictSet, sliceSegmentMap));
	}
	
	*_constraints = *constraints;
}

CoreSolver::EndExtractor::EndExtractor()
{
	registerInput(_eeSegments, "segments");
	registerInput(_eeSlices, "slices");
	registerOutput(_allSegments, "segments");
}

void CoreSolver::EndExtractor::updateOutputs()
{
	unsigned int z = 0;
	SegmentSet segmentSet;
	boost::shared_ptr<Segments> outputSegments = boost::make_shared<Segments>();
	
	LOG_DEBUG(coresolverlog) << "End extractor recieved " << outputSegments->size() <<
		" segments" << std::endl;
	
	foreach (boost::shared_ptr<Slice> slice, *_eeSlices)
	{
		if (slice->getSection() > z)
		{
			z = slice->getSection();
		}
	}
	
	LOG_DEBUG(coresolverlog) << " End extractor: max z is " << z << std::endl;

	foreach (boost::shared_ptr<Segment> segment, _eeSegments->getSegments())
	{
		segmentSet.add(segment);
	}

	
	foreach (boost::shared_ptr<Slice> slice, *_eeSlices)
	{
		if (slice->getSection() == z)
		{
			int begSize = segmentSet.size();
			boost::shared_ptr<EndSegment> rightEnd =
				boost::make_shared<EndSegment>(Segment::getNextSegmentId(),
											   Left,
											   slice);
			boost::shared_ptr<EndSegment> leftEnd =
				boost::make_shared<EndSegment>(Segment::getNextSegmentId(),
											   Right,
											   slice);
			
			segmentSet.add(rightEnd);
			segmentSet.add(leftEnd);
			
			LOG_ALL(coresolverlog) << "Added " << (segmentSet.size() - begSize) << " segments" << std::endl;
		}
	}
	
	
	foreach (boost::shared_ptr<Segment> segment, segmentSet)
	{
		outputSegments->add(segment);
	}
	
	LOG_DEBUG(coresolverlog) << "End extractor returning " << outputSegments->size() <<
		" segments" << std::endl;
	
	*_allSegments = *outputSegments;
}

CoreSolver::FeatureAssembler::FeatureAssembler()
{
	registerInput(_faSegments, "segments");
	registerInput(_faSegmentStore, "segment store");
	registerOutput(_features, "features");
}

void CoreSolver::FeatureAssembler::updateOutputs()
{

}


void
CoreSolver::updateOutputs()
{
	// A whole mess of pipeline variables
	boost::shared_ptr<SliceReader> sliceReader = boost::make_shared<SliceReader>();
	boost::shared_ptr<SegmentReader> segmentReader = boost::make_shared<SegmentReader>();
	boost::shared_ptr<ProblemAssembler> problemAssembler = boost::make_shared<ProblemAssembler>();
	boost::shared_ptr<ObjectiveGenerator> objectiveGenerator =
		boost::make_shared<ObjectiveGenerator>();
	boost::shared_ptr<SegmentationCostFunction> segmentationCostFunction =
		boost::make_shared<SegmentationCostFunction>();
	boost::shared_ptr<LinearSolver> linearSolver = boost::make_shared<LinearSolver>();
	boost::shared_ptr<Reconstructor> reconstructor = boost::make_shared<Reconstructor>();
	boost::shared_ptr<NeuronExtractor> neuronExtractor = boost::make_shared<NeuronExtractor>();
	boost::shared_ptr<ConstraintAssembler> constraintAssembler =
		boost::make_shared<ConstraintAssembler>();
	boost::shared_ptr<LinearCostFunction> linearCostFunction = boost::make_shared<LinearCostFunction>();;
	boost::shared_ptr<FileContentProvider> contentProvider
				= boost::make_shared<FileContentProvider>(
					optionLinearCostFunctionParametersFileBlock.as<std::string>());
	boost::shared_ptr<LinearCostFunctionParametersReader> reader
				= boost::make_shared<LinearCostFunctionParametersReader>();
	boost::shared_ptr<SegmentFeaturesExtractor> segmentFeaturesExtractor =
		boost::make_shared<SegmentFeaturesExtractor>();
	boost::shared_ptr<EndExtractor> endExtractor = boost::make_shared<EndExtractor>();

	// inputs and outputs
	boost::shared_ptr<LinearSolverParameters> binarySolverParameters = 
		boost::make_shared<LinearSolverParameters>(Binary);
	pipeline::Value<SegmentTrees> neurons;
	pipeline::Value<Blocks> boundingBlocks;
	pipeline::Value<Segments> segments;

	LOG_DEBUG(coresolverlog) << "Variables instantiated, setting up pipeline" << std::endl;
	
	segmentReader->setInput("blocks", _blocks);
	sliceReader->setInput("blocks", _blocks);
	
	segmentReader->setInput("store", _segmentStore);
	sliceReader->setInput("store", _sliceStore);
	
	endExtractor->setInput("segments", segmentReader->getOutput("segments"));
	endExtractor->setInput("slices", sliceReader->getOutput("slices"));
	
	constraintAssembler->setInput("segments", endExtractor->getOutput("segments"));
	constraintAssembler->setInput("conflict sets", sliceReader->getOutput("conflict sets"));
	constraintAssembler->setInput("force explanation", _forceExplanation);
	
	problemAssembler->addInput("neuron segments", endExtractor->getOutput("segments"));
	problemAssembler->addInput("neuron linear constraints", constraintAssembler->getOutput("linear constraints"));
	

	// Use problem assembler output to compute the bounding box we need to contain all
	// of the slices that are present. This will usually be larger than the requested
	// Blocks.
	boundingBlocks = computeBound(problemAssembler);
	pipeline::Value<util::point3<unsigned int> > offset(boundingBlocks->location());;
	
	LOG_DEBUG(coresolverlog) << "Segment bound computed" << *boundingBlocks << std::endl;
	
	objectiveGenerator->setInput("segments", problemAssembler->getOutput("segments"));
		
	segmentFeaturesExtractor->setInput("segments", problemAssembler->getOutput("segments"));
	segmentFeaturesExtractor->setInput("raw sections", _rawImageStore->getImageStack(*boundingBlocks));
	segmentFeaturesExtractor->setInput("crop offset", offset);
	
	reader->setInput(contentProvider->getOutput());
	linearCostFunction->setInput("features", segmentFeaturesExtractor->getOutput("all features"));
	linearCostFunction->setInput("parameters", reader->getOutput());

	objectiveGenerator->addInput("cost functions", linearCostFunction->getOutput("cost function"));
	
	
	linearSolver->setInput("objective", objectiveGenerator->getOutput());
	linearSolver->setInput("linear constraints", problemAssembler->getOutput("linear constraints"));
	linearSolver->setInput("parameters", binarySolverParameters);
	
	reconstructor->setInput("segments", problemAssembler->getOutput("segments"));
	reconstructor->setInput("solution", linearSolver->getOutput());

	neuronExtractor->setInput("segments", reconstructor->getOutput());
	
	LOG_DEBUG(coresolverlog) << "Pipeline is setup, extracting neurons" << std::endl;
	
	neurons = neuronExtractor->getOutput();
	segments = problemAssembler->getOutput("segments");
	
	*_outputSegments = *segments;
	*_neurons = *neurons;
}

pipeline::Value<Blocks>
CoreSolver::computeBound(const boost::shared_ptr<ProblemAssembler> problemAssembler)
{
		pipeline::Value<Segments> segments = problemAssembler->getOutput("segments");
	if (segments->size() > 0)
	{
		util::rect<int> bound =
			segments->getSegments()[0]->getSlices()[0]->getComponent()->getBoundingBox();
		boost::shared_ptr<Box<> > box;
		pipeline::Value<Blocks> boundBlocks;
		
		foreach (boost::shared_ptr<Segment> segment, segments->getSegments())
		{
			foreach (boost::shared_ptr<Slice> slice, segment->getSlices())
			{
				util::rect<int> componentBound = slice->getComponent()->getBoundingBox();
				bound.fit(componentBound);
			}
		}
		
		box = boost::make_shared<Box<> >(bound, _blocks->location().z, _blocks->size().z);
		
		*boundBlocks = *(_blocks->getManager()->blocksInBox(box));
		
		boundBlocks->expand(util::point3<int>(0,0,1));
		return boundBlocks;
	}
	else
	{
		LOG_DEBUG(coresolverlog) << "No segments, returning request bound." << std::endl;
		return _blocks;
	}
}


