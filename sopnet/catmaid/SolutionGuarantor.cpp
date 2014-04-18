#include <boost/make_shared.hpp>

#include <catmaid/EndExtractor.h>
#include <catmaid/persistence/SegmentFeatureReader.h>
#include <catmaid/persistence/SegmentReader.h>
#include <catmaid/persistence/SliceReader.h>
#include <catmaid/persistence/SolutionWriter.h>
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


#include "SolutionGuarantor.h"

using std::vector;
using std::map;

logger::LogChannel solutionguarantorlog("solutionguarantorlog", "[SolutionGuarantor] ");

util::ProgramOption optionLinearCostFunctionParametersFileSolutionGuarantor(
		util::_module           = "solutionGuarantor",
		util::_long_name        = "linearCostFunctionParameters",
		util::_description_text = "Path to a file containing the weights for the linear cost function.",
		util::_default_value    = "./feature_weights.dat");

util::ProgramOption optionRandomForestFileSolutionGuarantor(
		util::_module           = "solutionGuarantor",
		util::_long_name        = "segmentRandomForest",
		util::_description_text = "Path to an HDF5 file containing the segment random forest.",
		util::_default_value    = "segment_rf.hdf");

SolutionGuarantor::SolutionGuarantor()
{
	registerInput(_priorCostFunctionParameters, "prior cost parameters");
	registerInput(_cores, "cores");
	
	registerInput(_segmentationCostFunctionParameters, "segmentation cost parameters",
				pipeline::Optional);
	
	registerInput(_segmentStore, "segment store");
	registerInput(_sliceStore, "slice store");
	registerInput(_rawImageStore, "raw image store");
	registerInput(_membraneStore, "membrane image store");
	registerInput(_forceExplanation, "force explanation");
	
	registerOutput(_needBlocks, "need blocks");
}

SolutionGuarantor::ConstraintAssembler::ConstraintAssembler()
{
	registerInput(_segments, "segments");
	registerInput(_conflictSets, "conflict sets");
	registerInput(_assemblerForceExplanation, "force explanation");
	registerOutput(_constraints, "linear constraints");
}


boost::shared_ptr<LinearConstraint>
SolutionGuarantor::ConstraintAssembler::assembleConstraint(const ConflictSet& conflictSet,
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
	
	LOG_ALL(solutionguarantorlog) << "created constraint " << *constraint << std::endl;
	
	return constraint;
}

void SolutionGuarantor::ConstraintAssembler::updateOutputs()
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

void
SolutionGuarantor::updateOutputs()
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
					optionLinearCostFunctionParametersFileSolutionGuarantor.as<std::string>());
	boost::shared_ptr<LinearCostFunctionParametersReader> reader
				= boost::make_shared<LinearCostFunctionParametersReader>();
	boost::shared_ptr<SegmentFeatureReader> segmentFeatureReader =
		boost::make_shared<SegmentFeatureReader>();
	
	boost::shared_ptr<EndExtractor> endExtractor = boost::make_shared<EndExtractor>();
	
	boost::shared_ptr<SolutionWriter> solutionWriter = boost::make_shared<SolutionWriter>();
	
	// inputs and outputs
	boost::shared_ptr<LinearSolverParameters> binarySolverParameters = 
		boost::make_shared<LinearSolverParameters>(Binary);
	pipeline::Value<SegmentTrees> neurons;
	pipeline::Value<Segments> segments;
	boost::shared_ptr<Blocks> blocks = _cores->asBlocks();
	boost::shared_ptr<Blocks> needBlocks = boost::make_shared<Blocks>();

	LOG_DEBUG(solutionguarantorlog) << "Variables instantiated, setting up pipeline" << std::endl;
	
	segmentReader->setInput("blocks", blocks);
	sliceReader->setInput("blocks", blocks);
	
	segmentReader->setInput("store", _segmentStore);
	sliceReader->setInput("store", _sliceStore);
	
	endExtractor->setInput("segments", segmentReader->getOutput("segments"));
	endExtractor->setInput("slices", sliceReader->getOutput("slices"));
	
	constraintAssembler->setInput("segments", endExtractor->getOutput("segments"));
	constraintAssembler->setInput("conflict sets", sliceReader->getOutput("conflict sets"));
	constraintAssembler->setInput("force explanation", _forceExplanation);
	
	problemAssembler->addInput("neuron segments", endExtractor->getOutput("segments"));
	problemAssembler->addInput("neuron linear constraints", constraintAssembler->getOutput("linear constraints"));
	
	segmentFeatureReader->setInput("segments", endExtractor->getOutput("segments"));
	segmentFeatureReader->setInput("store", _segmentStore);
	segmentFeatureReader->setInput("block manager", blocks->getManager());
	segmentFeatureReader->setInput("raw stack store", _rawImageStore);
	
	objectiveGenerator->setInput("segments", problemAssembler->getOutput("segments"));
	
	reader->setInput(contentProvider->getOutput());
	
	linearCostFunction->setInput("features", segmentFeatureReader->getOutput("features"));
	linearCostFunction->setInput("parameters", reader->getOutput());

	objectiveGenerator->addInput("cost functions", linearCostFunction->getOutput("cost function"));
	
	
	linearSolver->setInput("objective", objectiveGenerator->getOutput());
	linearSolver->setInput("linear constraints", problemAssembler->getOutput("linear constraints"));
	linearSolver->setInput("parameters", binarySolverParameters);
	
	solutionWriter->setInput("segments", problemAssembler->getOutput("segments"));
	solutionWriter->setInput("cores", _cores);
	solutionWriter->setInput("solution", linearSolver->getOutput());
	solutionWriter->setInput("store", _segmentStore);
	
	LOG_DEBUG(solutionguarantorlog) << "Pipeline is setup, extracting neurons" << std::endl;

	solutionWriter->writeSolution();
	
	*_needBlocks = *needBlocks;
}

pipeline::Value<Blocks> SolutionGuarantor::guaranteeSolution()
{
	return pipeline::Value<Blocks>();
}

boost::shared_ptr<Blocks>
SolutionGuarantor::bufferCore(boost::shared_ptr<Core> core, unsigned int buffer)
{
	pipeline::Value<Cores> cores;
	cores->add(core);
	return bufferCores(cores, buffer);
}

boost::shared_ptr<Blocks>
SolutionGuarantor::bufferCores(boost::shared_ptr<Cores> cores, unsigned int buffer)
{
	boost::shared_ptr<Blocks> blocks = boost::make_shared<Blocks>(cores->asBlocks());

	for (unsigned int i = 0; i < buffer; ++i)
	{
		blocks->dilateXY();
	}

	return blocks;
}