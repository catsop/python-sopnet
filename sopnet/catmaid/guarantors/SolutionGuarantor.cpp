#include <pipeline/Process.h>
#include <pipeline/Value.h>
#include <catmaid/persistence/SegmentDescriptions.h>
#include <catmaid/persistence/exceptions.h>
#include <catmaid/blocks/Cores.h>
#include <inference/LinearSolver.h>
#include <util/Logger.h>
#include "SolutionGuarantor.h"

logger::LogChannel solutionguarantorlog("solutionguarantorlog", "[SolutionGuarantor] ");

SolutionGuarantor::SolutionGuarantor(
		const ProjectConfiguration&     projectConfiguration,
		boost::shared_ptr<SegmentStore> segmentStore,
		boost::shared_ptr<SliceStore>   sliceStore,
		unsigned int                    corePadding,
		const std::vector<double>&      featureWeights) :
	_segmentStore(segmentStore),
	_sliceStore(sliceStore),
	_corePadding(corePadding),
	_weights(featureWeights),
	_blockUtils(projectConfiguration) {

	if (_corePadding == 0)
		UTIL_THROW_EXCEPTION(
				UsageError,
				"the core padding must be at least 1");
}


Blocks
SolutionGuarantor::guaranteeSolution(const Core& core) {

	LOG_DEBUG(solutionguarantorlog)
			<< "requesting solution for core ("
			<< core.x() << ", " << core.y() << ", " << core.z()
			<< ")" << std::endl;

	// get all the blocks in the padded core
	Blocks blocks = getPaddedCoreBlocks(core);

	LOG_DEBUG(solutionguarantorlog) << "with padding this corresponds to blocks " << blocks << std::endl;

	// get all segments for these blocks
	Blocks missingBlocks;
	boost::shared_ptr<SegmentDescriptions> segments = _segmentStore->getSegmentsByBlocks(blocks, missingBlocks);

	if (!missingBlocks.empty())
		return missingBlocks;

	// get all conflict sets for these blocks
	boost::shared_ptr<ConflictSets> conflictSets = _sliceStore->getConflictSetsByBlocks(blocks, missingBlocks);

	if (!missingBlocks.empty())
		UTIL_THROW_EXCEPTION(
				CorruptedDatabaseException,
				"the segment store does contain segments for the requested blocks, "
				"but the slice store reports that no conflict sets have been extracted");

	// compute solution
	std::vector<SegmentHash> solution = computeSolution(*segments, *conflictSets);

	// store solution
	_segmentStore->storeSolution(solution, core);

	// there are no missing blocks
	return Blocks();
}

Blocks
SolutionGuarantor::getPaddedCoreBlocks(const Core& core) {

	// get the core blocks
	Blocks blocks = _blockUtils.getCoreBlocks(core);

	// grow by _corePadding in each direction
	_blockUtils.expand(
			blocks,
			_corePadding, _corePadding, _corePadding,
			_corePadding, _corePadding, _corePadding);

	return blocks;
}

std::vector<SegmentHash>
SolutionGuarantor::computeSolution(
		const SegmentDescriptions& segments,
		const ConflictSets& conflictSets) {

	// create the hash <-> variable mappings and the slice -> [segments] 
	// mappings

	unsigned int nextVar = 0;
	foreach (const SegmentDescription& segment, segments) {

		SegmentHash segmentHash = segment.getHash();

		_hashToVariable[segmentHash] = nextVar;
		_variableToHash[nextVar] = segmentHash;

		foreach (SliceHash leftSliceHash, segment.getLeftSlices())
			_leftSliceToSegments[leftSliceHash].push_back(segmentHash);

		foreach (SliceHash rightSliceHash, segment.getRightSlices())
			_rightSliceToSegments[rightSliceHash].push_back(segmentHash);
	}

	// create linear constraints on the variables

	boost::shared_ptr<LinearConstraints> constraints = createConstraints(segments, conflictSets);

	// create the cost function

	boost::shared_ptr<LinearObjective> objective = createObjective(segments);

	// solve the ILP

	pipeline::Process<LinearSolver> solver;

	boost::shared_ptr<LinearSolverParameters> parameters =
			boost::make_shared<LinearSolverParameters>(Binary);

	solver->setInput("objective", objective);
	solver->setInput("linear constraints", constraints);
	solver->setInput("parameters", parameters);

	pipeline::Value<Solution> solution = solver->getOutput();

	// find the segment hashes that correspond to the solution

	std::vector<SegmentHash> solutionSegments;

	for (unsigned int var = 0; var < nextVar; var++)
		if ((*solution)[var] == 1.0)
			solutionSegments.push_back(_variableToHash[var]);

	return solutionSegments;
}

boost::shared_ptr<LinearConstraints>
SolutionGuarantor::createConstraints(
		const SegmentDescriptions& segments,
		const ConflictSets&        conflictSets) {

	boost::shared_ptr<LinearConstraints> constraints = boost::make_shared<LinearConstraints>();

	addOverlapConstraints(segments, conflictSets, *constraints);
	addContinuationConstraints(segments, *constraints);

	return constraints;
}

boost::shared_ptr<LinearObjective>
SolutionGuarantor::createObjective(const SegmentDescriptions& segments) {

	LOG_DEBUG(solutionguarantorlog) << "creating objective for " << segments.size() << " segments" << std::endl;

	boost::shared_ptr<LinearObjective> objective = boost::make_shared<LinearObjective>(segments.size());

	foreach (const SegmentDescription& segment, segments) {

		unsigned int var  = _hashToVariable[segment.getHash()];
		double       cost = getCost(segment.getFeatures());

		objective->setCoefficient(var, cost);
	}

	return objective;
}

void
SolutionGuarantor::addOverlapConstraints(
		const SegmentDescriptions& segments,
		const ConflictSets&        conflictSets,
		LinearConstraints&         constraints) {

	// for each conflict set:
	foreach (const ConflictSet& conflictSet, conflictSets) {

		LinearConstraint constraint;

		// get all segments that use the conflict slices on the left and require 
		// the sum of their variables to be at most one

		foreach (const SliceHash& sliceHash, conflictSet.getSlices()) {

			foreach (const SegmentHash& segmentHash, _leftSliceToSegments[sliceHash]) {

				unsigned int var = _hashToVariable[segmentHash];

				constraint.setCoefficient(var, 1.0);
			}
		}

		constraint.setRelation(LessEqual);
		constraint.setValue(1.0);

		constraints.add(constraint);
	}
}

void
SolutionGuarantor::addContinuationConstraints(
		const SegmentDescriptions& segments,
		LinearConstraints&         constraints) {

	typedef std::map<SliceHash, std::vector<SegmentHash> >::value_type SliceToSegments;

	// for each slice
	foreach (const SliceToSegments& sliceToSegments, _rightSliceToSegments) {

		SliceHash sliceHash = sliceToSegments.first;

		// get all segments that use this slice from the left
		const std::vector<SegmentHash>& leftSegments = sliceToSegments.second;

		// get all segments that use this slice from the right
		const std::vector<SegmentHash>& rightSegments = _leftSliceToSegments[sliceHash];

		// require the sum of their variables to be equal

		LinearConstraint constraint;

		foreach (SegmentHash segmentHash, leftSegments)
			constraint.setCoefficient(_hashToVariable[segmentHash], 1.0);
		foreach (SegmentHash segmentHash, rightSegments)
			constraint.setCoefficient(_hashToVariable[segmentHash], -1.0);

		constraint.setRelation(Equal);
		constraint.setValue(0.0);

		constraints.add(constraint);
	}
}

double
SolutionGuarantor::getCost(const std::vector<double>& features) {

	double cost = 0;

	std::vector<double>::const_iterator i = features.begin();
	std::vector<double>::const_iterator j = _weights.begin();

	while (i != features.end()) {

		cost += (*i)*(*j);
		i++;
		j++;
	}

	return cost;
}
