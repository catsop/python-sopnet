#include <boost/make_shared.hpp>

#include <catmaid/EndExtractor.h>
#include <catmaid/persistence/SegmentFeatureReader.h>
#include <catmaid/persistence/SegmentSolutionWriter.h>
#include <catmaid/persistence/CostReader.h>
#include <catmaid/persistence/CostWriter.h>
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
	registerInput(_inCores, "cores");
	
	registerInput(_segmentationCostFunctionParameters, "segmentation cost parameters",
				pipeline::Optional);
	
	registerInput(_segmentStore, "segment store");
	registerInput(_sliceStore, "slice store");
	registerInput(_rawImageStore, "raw stack store");
	registerInput(_membraneStore, "membrane stack store");
	registerInput(_forceExplanation, "force explanation");
	registerInput(_buffer, "buffer");
	
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
	_constraints = new LinearConstraints();
	
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
		_constraints->add(*assembleConstraint(conflictSet, sliceSegmentMap));
	}
}

SolutionGuarantor::LinearObjectiveAssembler::LinearObjectiveAssembler()
{
	
	registerInput(_store, "segment store");
	registerInput(_segments, "segments");
	registerInput(_blocks, "blocks");
	registerInput(_rawStackStore, "stack store");
	
	registerOutput(_objective, "objective");
}

void
SolutionGuarantor::LinearObjectiveAssembler::updateOutputs()
{
	
	pipeline::Value<Segments> noCostSegments;
	pipeline::Value<LinearObjective> objective;
	boost::shared_ptr<CostReader> costReader = boost::make_shared<CostReader>();
	
	// Retrieve LinearObjective costs from the store.
	
	costReader->setInput("store", _store);
	costReader->setInput("segments", _segments);
	
	objective = costReader->getOutput("objective");
	noCostSegments = costReader->getOutput("costless segments");

	_objective = new LinearObjective();
	*_objective = *objective;

	
	LOG_DEBUG(solutionguarantorlog) << "Read no objective coefficients for " <<
		noCostSegments->size() << " of " << _segments->size() << " segments" << std::endl;
	
	if (noCostSegments->size() > 0)
	{
		// When noCostSegments is non-empty, there are Segments for which
		// no cost was retrieved.
		std::string filename =
			optionLinearCostFunctionParametersFileSolutionGuarantor.as<std::string>();
			
		boost::shared_ptr<LinearCostFunctionParametersReader> reader
				= boost::make_shared<LinearCostFunctionParametersReader>(filename);
		boost::shared_ptr<SegmentFeatureReader> segmentFeatureReader =
			boost::make_shared<SegmentFeatureReader>();
		boost::shared_ptr<LinearCostFunction> linearCostFunction =
			boost::make_shared<LinearCostFunction>();
		boost::shared_ptr<ObjectiveGenerator> objectiveGenerator =
			boost::make_shared<ObjectiveGenerator>();
		boost::shared_ptr<CostWriter> costWriter = boost::make_shared<CostWriter>();
		boost::unordered_map<boost::shared_ptr<Segment>, double,
							 SegmentPointerHash, SegmentPointerEquals> segmentCostMap;
		std::vector<boost::shared_ptr<Segment> > segmentVector = noCostSegments->getSegments();
		std::vector<double> coefficientVector;
		unsigned int i = 0;
		pipeline::Value<LinearObjective> computedObjective;
		pipeline::Value<Features> features;
		pipeline::Value<LinearCostFunctionParameters> costFunctionParams = reader->getOutput();
		
		// Compute the objective for the costless segments.
		//reader->setInput(paramFileStream);
		
		segmentFeatureReader->setInput("segments", noCostSegments);
		segmentFeatureReader->setInput("store", _store);
		segmentFeatureReader->setInput("block manager", _blocks->getManager());
		segmentFeatureReader->setInput("raw stack store", _rawStackStore);
		
		features = segmentFeatureReader->getOutput();
		
		linearCostFunction->setInput("features", features);
		linearCostFunction->setInput("parameters", costFunctionParams);
		
		objectiveGenerator->setInput("segments", noCostSegments);
		objectiveGenerator->addInput("cost functions",
									 linearCostFunction->getOutput("cost function"));
		
		computedObjective = objectiveGenerator->getOutput();
		coefficientVector = computedObjective->getCoefficients();
		
		// Build a map from segment pointer to cost.
		// Here, we assume that the ObjectiveGenerator produces a LinearObjective object with
		// the same size as Segments.
		for (unsigned int j = 0; j < segmentVector.size(); ++j)
		{
			segmentCostMap[segmentVector[j]] = coefficientVector[j];
		}
		
		// Finally, iterate through the list of all Segments
		// Here, we assume that _segments and obective are co-indexed.
		foreach (boost::shared_ptr<Segment> segment, _segments->getSegments())
		{
			// If segment is mapped in the segmentCostMap, then it also exists in noCostSegments
			// IE, its coefficient is in computedObjective, but not in objective.
			if (segmentCostMap.count(segment))
			{
				// So replace the dummy entry in objective with the real one
				_objective->setCoefficient(i, segmentCostMap[segment]);
			}
			++i;
		}
		
		// Now, as a post script, we write the Objective to the SegmentStore
		costWriter->setInput("store", _store);
		costWriter->setInput("segments", noCostSegments);
		costWriter->setInput("objective", computedObjective);
		costWriter->writeCosts();
	}
}



void
SolutionGuarantor::solve()
{
	// A whole mess of pipeline variables
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
	boost::shared_ptr<LinearCostFunction> linearCostFunction = boost::make_shared<LinearCostFunction>();
	boost::shared_ptr<FileContentProvider> contentProvider
				= boost::make_shared<FileContentProvider>(
					optionLinearCostFunctionParametersFileSolutionGuarantor.as<std::string>());
	boost::shared_ptr<LinearCostFunctionParametersReader> reader
				= boost::make_shared<LinearCostFunctionParametersReader>();
	boost::shared_ptr<SegmentFeatureReader> segmentFeatureReader =
		boost::make_shared<SegmentFeatureReader>();
	
	boost::shared_ptr<EndExtractor> endExtractor = boost::make_shared<EndExtractor>();
	
	boost::shared_ptr<SegmentSolutionWriter> solutionWriter = boost::make_shared<SegmentSolutionWriter>();
	
	boost::shared_ptr<LinearObjectiveAssembler> objectiveAssembler =
		boost::make_shared<LinearObjectiveAssembler>();
	
	// inputs and outputs
	boost::shared_ptr<LinearSolverParameters> binarySolverParameters = 
		boost::make_shared<LinearSolverParameters>(Binary);
	pipeline::Value<SegmentTrees> neurons;
	boost::shared_ptr<Blocks> needBlocks = boost::make_shared<Blocks>();

	LOG_DEBUG(solutionguarantorlog) << "Variables instantiated, setting up pipeline" << std::endl;

	boost::shared_ptr<Slices>       slices       = _sliceStore->retrieveSlices(*_bufferedBlocks);
	boost::shared_ptr<ConflictSets> conflictSets = _sliceStore->retrieveConflictSets(*slices);
	boost::shared_ptr<Segments>     segments     = _segmentStore->retrieveSegments(*_bufferedBlocks);

	endExtractor->setInput("slices", slices);
	endExtractor->setInput("segments", segments);
	
	constraintAssembler->setInput("segments", endExtractor->getOutput("segments"));
	constraintAssembler->setInput("conflict sets", conflictSets);
	constraintAssembler->setInput("force explanation", _forceExplanation);
	
	problemAssembler->addInput("neuron segments", endExtractor->getOutput("segments"));
	problemAssembler->addInput("neuron linear constraints", constraintAssembler->getOutput("linear constraints"));

	objectiveAssembler->setInput("segment store", _segmentStore);
	objectiveAssembler->setInput("segments", endExtractor->getOutput("segments"));
	objectiveAssembler->setInput("blocks", _bufferedBlocks);
	objectiveAssembler->setInput("stack store", _rawImageStore);

// 	segmentFeatureReader->setInput("segments", endExtractor->getOutput("segments"));
// 	segmentFeatureReader->setInput("store", _segmentStore);
// 	segmentFeatureReader->setInput("block manager", _bufferedBlocks->getManager());
// 	segmentFeatureReader->setInput("raw stack store", _rawImageStore);
	
// 	objectiveGenerator->setInput("segments", problemAssembler->getOutput("segments"));
	
// 	reader->setInput(contentProvider->getOutput());
	
// 	linearCostFunction->setInput("features", segmentFeatureReader->getOutput("features"));
// 	linearCostFunction->setInput("parameters", reader->getOutput());

// 	objectiveGenerator->addInput("cost functions", linearCostFunction->getOutput("cost function"));
	
	
	linearSolver->setInput("objective", objectiveAssembler->getOutput());
	linearSolver->setInput("linear constraints", problemAssembler->getOutput("linear constraints"));
	linearSolver->setInput("parameters", binarySolverParameters);
	
	solutionWriter->setInput("segments", problemAssembler->getOutput("segments"));
	solutionWriter->setInput("cores", _inCores);
	solutionWriter->setInput("solution", linearSolver->getOutput());
	solutionWriter->setInput("store", _segmentStore);
	
	LOG_DEBUG(solutionguarantorlog) << "Pipeline is setup, extracting neurons" << std::endl;

	solutionWriter->writeSolution();
}

void
SolutionGuarantor::updateOutputs()
{
	pipeline::Value<Blocks> needBlocks = guaranteeSolution();
	
	_needBlocks = new Blocks();
	*_needBlocks = *needBlocks;
}

pipeline::Value<Blocks> 
SolutionGuarantor::checkBlocks()
{
	pipeline::Value<Blocks> needBlocks = pipeline::Value<Blocks>();
	
	foreach (boost::shared_ptr<Block> block, *_bufferedBlocks)
	{
		if (!block->getSegmentsFlag() || !block->getSlicesFlag())
		{
			needBlocks->add(block);
		}
	}
	
	return needBlocks;
}

pipeline::Value<Blocks> SolutionGuarantor::guaranteeSolution()
{
	pipeline::Value<Blocks> needBlocks;
	
	updateInputs();

	LOG_USER(solutionguarantorlog) << "guaranteeSolution called for cores:" << std::endl;
	foreach (boost::shared_ptr<Core> core, *_inCores)
		LOG_USER(solutionguarantorlog) << "\t" << core->getCoordinates() << std::endl;
	
	_bufferedBlocks = bufferCores(_inCores, *_buffer);

	LOG_USER(solutionguarantorlog) << "with padding, this is the following blocks:" << std::endl;
	foreach (boost::shared_ptr<Block> block, *_bufferedBlocks)
		LOG_USER(solutionguarantorlog) << "\t" << block->getCoordinates() << std::endl;
	
	needBlocks = checkBlocks();

	LOG_USER(solutionguarantorlog) << "of those blocks, the following don't have their segments, yet:" << std::endl;
	foreach (boost::shared_ptr<Block> block, *needBlocks)
		LOG_USER(solutionguarantorlog) << "\t" << block->getCoordinates() << std::endl;
	
	if (needBlocks->empty())
	{
		solve();
	}
	
	return needBlocks;
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
	LOG_USER(solutionguarantorlog) << "cores consist of:" << std::endl;
	foreach (boost::shared_ptr<Core> core, *cores) {
		LOG_USER(solutionguarantorlog) << "\tcore at " << core->getCoordinates() << " with:" << std::endl;
		foreach (boost::shared_ptr<Block> block, *core) {
			LOG_USER(solutionguarantorlog) << "\t\tblock at " << block->getCoordinates() << std::endl;
		}
	}

	boost::shared_ptr<Blocks> blocks = boost::make_shared<Blocks>(cores->asBlocks());

	blocks->dilate(buffer, buffer, buffer);

	return blocks;
}
