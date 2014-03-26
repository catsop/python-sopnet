#include "SolutionGuarantor.h"
#include <sopnet/segments/SegmentSet.h>

#include <catmaid/persistence/CostWriter.h>
#include <catmaid/persistence/CostReader.h>
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

using std::vector;
using std::map;

logger::LogChannel solutionguarantorlog("solutionguarantorlog", "[SolutionGuarantor] ");

util::ProgramOption optionCoreLinearCostFunctionParametersFile(
		util::_module           = "core",
		util::_long_name        = "linearCostFunctionParameters",
		util::_description_text = "Path to a file containing the weights for the linear cost function.",
		util::_default_value    = "./feature_weights.dat");

util::ProgramOption optionCoreForceExplanation(
		util::_module           = "core",
		util::_long_name        = "forceExplanation",
		util::_description_text = "Force explanation, default true",
		util::_default_value    = true);

util::ProgramOption optionCoreBufferRadius(
		util::_module           = "core",
		util::_long_name        = "bufferRadius",
		util::_description_text = "Buffer Radius, in Blocks",
		util::_default_value    = "2");

SolutionGuarantor::SolutionGuarantor()
{
	registerInput(_cores, "cores");
	registerInput(_segmentStore, "segment store");
	registerInput(_sliceStore, "slice store");
	registerInput(_rawImageStore, "raw image store");
	registerInput(_membraneStore, "membrane image store");
	
	registerInput(_bufferRadius, "buffer", pipeline::Optional);
	registerInput(_forceExplanation, "force explanation", pipeline::Optional);
	registerInput(_segmentationCostFunctionParameters, "segmentation cost parameters",
				pipeline::Optional);
	registerInput(_priorCostFunctionParameters, "prior cost parameters", pipeline::Optional);
	
	registerOutput(_needBlocks, "need blocks");
}

pipeline::Value<Blocks>
SolutionGuarantor::guaranteeSolution()
{
	pipeline::Value<Blocks> needBlocks;
	boost::shared_ptr<Blocks> solveBlocks;
	
	updateInputs();
	
	setupInputs();
	
	solveBlocks = boost::make_shared<Blocks>(_cores->asBlocks());
	
	for (int i = 0; i < _useBufferRadius; ++i)
	{
		solveBlocks->dilateXY();
	}
	
	//TODO: decide whether or not this should go in the for loop above
	solveBlocks->expand(util::point3<int>(0, 0, 1));
	solveBlocks->expand(util::point3<int>(0, 0, -1));
	
	// checkData will add Blocks to needBlocks if we need segments to be extracted
	if (checkData(solveBlocks, needBlocks))
	{
		solve(solveBlocks);
	}
	
	return needBlocks;
}

void SolutionGuarantor::updateOutputs()
{
	pipeline::Value<Blocks> needBlocks = guaranteeSolution();
	
	*_needBlocks = *needBlocks;
}

bool
SolutionGuarantor::checkData(const boost::shared_ptr<Blocks> solveBlocks,
							 pipeline::Value<Blocks> needBlocks)
{
	bool ok = true;
	bool proceed = false;
	
	foreach (boost::shared_ptr<Core> core, *_cores)
	{
		if (!core->getSolutionSetFlag())
		{
			proceed = true;
			break;
		}
	}
	
	// If proceed was set true, then there is at least one Core on input that has not yet
	// had its solution set.
	// On the other hand, if proceed is false, then we have no work to do and can simply return.
	if (!proceed)
	{
		return false;
	}
	
	foreach (boost::shared_ptr<Block> block, *solveBlocks)
	{
		if (!block->getSegmentsFlag() || !block->getSlicesFlag())
		{
			ok = false;
			needBlocks->add(block);
		}
	}
	
	return ok;
}

boost::shared_ptr<Blocks>
SolutionGuarantor::checkCost(const boost::shared_ptr<Blocks> blocks)
{
	boost::shared_ptr<Blocks> needCostBlocks = boost::make_shared<Blocks>();
	
	foreach (boost::shared_ptr<Block> block, *blocks)
	{
		if (!block->getSolutionCostFlag())
		{
			needCostBlocks->add(block);
		}
	}
	
	return needCostBlocks;
}

void SolutionGuarantor::setupInputs()
{
	if (_forceExplanation)
	{
		_useForceExplanation = pipeline::Value<bool>(*_forceExplanation);
	}
	else
	{
		bool forceExplanation = optionCoreForceExplanation;
		_useForceExplanation = pipeline::Value<bool>(forceExplanation);
	}
	
	if (_bufferRadius)
	{
		_useBufferRadius = *_bufferRadius;
	}
	else
	{
		_useBufferRadius = optionCoreBufferRadius.as<unsigned int>();
	}
}

void SolutionGuarantor::solve(const boost::shared_ptr<Blocks> blocks)
{
	boost::shared_ptr<SliceReader> sliceReader = boost::make_shared<SliceReader>();
	boost::shared_ptr<SegmentReader> segmentReader = boost::make_shared<SegmentReader>();
	boost::shared_ptr<CostReader> costReader = boost::make_shared<CostReader>();
	boost::shared_ptr<SolutionWriter> solutionWriter = boost::make_shared<SolutionWriter>();
	boost::shared_ptr<LinearSolver> linearSolver = boost::make_shared<LinearSolver>();
	boost::shared_ptr<ConstraintAssembler> constraintAssembler =
		boost::make_shared<ConstraintAssembler>();
	boost::shared_ptr<EndExtractor> endExtractor = boost::make_shared<EndExtractor>();
	boost::shared_ptr<ProblemAssembler> problemAssembler = boost::make_shared<ProblemAssembler>();
	
	boost::shared_ptr<LinearSolverParameters> binarySolverParameters = 
		boost::make_shared<LinearSolverParameters>(Binary);
	
	segmentReader->setInput("blocks", blocks);
	sliceReader->setInput("blocks", blocks);
	
	segmentReader->setInput("store", _segmentStore);
	sliceReader->setInput("store", _sliceStore);
	
	endExtractor->setInput("segments", segmentReader->getOutput("segments"));
	endExtractor->setInput("slices", sliceReader->getOutput("slices"));
	
	constraintAssembler->setInput("segments", endExtractor->getOutput("segments"));
	constraintAssembler->setInput("conflict sets", sliceReader->getOutput("conflict sets"));
	constraintAssembler->setInput("force explanation", _useForceExplanation);
	
	problemAssembler->addInput("neuron segments", endExtractor->getOutput("segments"));
	problemAssembler->addInput("neuron linear constraints", constraintAssembler->getOutput("linear constraints"));

	costReader->setInput("store", _segmentStore);
	costReader->setInput("segments", problemAssembler->getOutput("segments"));
	
	linearSolver->setInput("objective", costReader->getOutput("objective"));
	linearSolver->setInput("linear constraints", problemAssembler->getOutput("linear constraints"));
	linearSolver->setInput("parameters", binarySolverParameters);

	solutionWriter->setInput("segments", problemAssembler->getOutput("segments"));
	solutionWriter->setInput("solution", linearSolver->getOutput("solution"));
	solutionWriter->setInput("cores", _cores);
	
	solutionWriter->writeSolution();
}


void SolutionGuarantor::storeCosts(const boost::shared_ptr<Blocks> costBlocks)
{
	boost::shared_ptr<Blocks> blocks = checkCost(costBlocks);
	
	// A whole mess of pipeline variables
	boost::shared_ptr<SliceReader> sliceReader = boost::make_shared<SliceReader>();
	boost::shared_ptr<SegmentReader> segmentReader = boost::make_shared<SegmentReader>();
	boost::shared_ptr<ProblemAssembler> problemAssembler = boost::make_shared<ProblemAssembler>();
	boost::shared_ptr<ObjectiveGenerator> objectiveGenerator =
		boost::make_shared<ObjectiveGenerator>();
	boost::shared_ptr<SegmentationCostFunction> segmentationCostFunction =
		boost::make_shared<SegmentationCostFunction>();
	boost::shared_ptr<ConstraintAssembler> constraintAssembler =
		boost::make_shared<ConstraintAssembler>();
	boost::shared_ptr<LinearCostFunction> linearCostFunction = boost::make_shared<LinearCostFunction>();;
	boost::shared_ptr<FileContentProvider> contentProvider
				= boost::make_shared<FileContentProvider>(
					optionCoreLinearCostFunctionParametersFile.as<std::string>());
	boost::shared_ptr<LinearCostFunctionParametersReader> reader
				= boost::make_shared<LinearCostFunctionParametersReader>();
	boost::shared_ptr<SegmentFeatureReader> segmentFeatureReader =
		boost::make_shared<SegmentFeatureReader>();
	
	boost::shared_ptr<EndExtractor> endExtractor = boost::make_shared<EndExtractor>();
	
	boost::shared_ptr<CostWriter> costWriter = boost::make_shared<CostWriter>();
	
	// inputs and outputs
	boost::shared_ptr<LinearSolverParameters> binarySolverParameters = 
		boost::make_shared<LinearSolverParameters>(Binary);
	pipeline::Value<SegmentTrees> neurons;
	pipeline::Value<Segments> segments;
	

	LOG_DEBUG(solutionguarantorlog) << "Variables instantiated, setting up pipeline" << std::endl;
	
	segmentReader->setInput("blocks", blocks);
	sliceReader->setInput("blocks", blocks);
	
	segmentReader->setInput("store", _segmentStore);
	sliceReader->setInput("store", _sliceStore);
	
	endExtractor->setInput("segments", segmentReader->getOutput("segments"));
	endExtractor->setInput("slices", sliceReader->getOutput("slices"));
	
	constraintAssembler->setInput("segments", endExtractor->getOutput("segments"));
	constraintAssembler->setInput("conflict sets", sliceReader->getOutput("conflict sets"));
	constraintAssembler->setInput("force explanation", _useForceExplanation);
	
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
	
// 	if (_segmentationCostFunctionParameters)
// 	{
// 		//TODO
// 	}
	
	costWriter->setInput("store", _segmentStore);
	costWriter->setInput("segments", problemAssembler->getOutput("segments"));
	costWriter->setInput("objective", objectiveGenerator->getOutput());
	
	costWriter->writeCosts();
	
	foreach (boost::shared_ptr<Block> block, *blocks)
	{
		block->setSolutionCostFlag(true);
	}
}


SolutionGuarantor::EndExtractor::EndExtractor()
{
	registerInput(_eeSegments, "segments");
	registerInput(_eeSlices, "slices");
	registerOutput(_allSegments, "segments");
}

void
SolutionGuarantor::EndExtractor::updateOutputs()
{
	unsigned int z = 0;
	SegmentSet segmentSet;
	boost::shared_ptr<Segments> outputSegments = boost::make_shared<Segments>();
	
	LOG_DEBUG(solutionguarantorlog) << "End extractor recieved " << _eeSegments->size() <<
		" segments" << std::endl;
	
	foreach (boost::shared_ptr<Slice> slice, *_eeSlices)
	{
		if (slice->getSection() > z)
		{
			z = slice->getSection();
		}
	}
	
	LOG_DEBUG(solutionguarantorlog) << " End extractor: max z is " << z << std::endl;

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
			
			LOG_ALL(solutionguarantorlog) << "Added " << (segmentSet.size() - begSize) << " segments" << std::endl;
		}
	}
	
	
	foreach (boost::shared_ptr<Segment> segment, segmentSet)
	{
		outputSegments->add(segment);
	}
	
	LOG_DEBUG(solutionguarantorlog) << "End extractor returning " << outputSegments->size() <<
		" segments" << std::endl;
	
	*_allSegments = *outputSegments;
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
							std::map<unsigned int, std::vector<unsigned int> >& sliceSegmentsMap)
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

void
SolutionGuarantor::ConstraintAssembler::updateOutputs()
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
