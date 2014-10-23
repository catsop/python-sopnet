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
		unsigned int                    corePadding) :
	_segmentStore(segmentStore),
	_sliceStore(sliceStore),
	_corePadding(corePadding),
	_blockUtils(projectConfiguration) {

	if (_corePadding == 0)
		UTIL_THROW_EXCEPTION(
				UsageError,
				"the core padding must be at least 1");

	LOG_DEBUG(solutionguarantorlog) << "core size is " << projectConfiguration.getCoreSize() << std::endl;
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

	boost::shared_ptr<SegmentConstraints> explicitConstraints = _segmentStore->getConstraintsByBlocks(blocks);

	LOG_DEBUG(solutionguarantorlog) << "computing solution..." << std::endl;

	_weights = _segmentStore->getFeatureWeights();

	// compute solution
	std::vector<SegmentHash> solution = computeSolution(*segments, *conflictSets, *explicitConstraints);

	LOG_DEBUG(solutionguarantorlog) << "solution contains " << solution.size() << " segments" << std::endl;

	// store solution
	_segmentStore->storeSolution(solution, core);

	LOG_DEBUG(solutionguarantorlog) << "done" << std::endl;

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
		const ConflictSets&        conflictSets,
		const SegmentConstraints&  explicitConstraints) {

	unsigned int firstSection = std::numeric_limits<unsigned int>::max();
	unsigned int lastSection  = 0;

	// get the first and last section
	foreach (const SegmentDescription& segment, segments) {

		firstSection = std::min(firstSection, segment.getSection());
		lastSection  = std::max(lastSection,  segment.getSection() + 1);
	}

	LOG_DEBUG(solutionguarantorlog)
			<< "first section is " << firstSection
			<< ", last section (inclusive) is "
			<< lastSection << std::endl;

	// create the hash <-> variable mappings and the slice -> [segments] 
	// mappings

	unsigned int numEnds          = 0;
	unsigned int numContinuations = 0;
	unsigned int numBranches      = 0;

	_rightSliceToSegments.clear();
	_leftSliceToSegments.clear();
	_slices.clear();
	_firstSlices.clear();
	_lastSlices.clear();
	_variableToHash.clear();

	unsigned int nextVar = 0;
	foreach (const SegmentDescription& segment, segments) {

		SegmentHash segmentHash = segment.getHash();

		_hashToVariable[segmentHash] = nextVar;
		_variableToHash[nextVar] = segmentHash;
		nextVar++;

		foreach (SliceHash leftSliceHash, segment.getLeftSlices()) {

			_leftSliceToSegments[leftSliceHash].push_back(segmentHash);
			_slices.insert(leftSliceHash);

			if (segment.getSection() == firstSection)
				_firstSlices.insert(leftSliceHash);
		}

		foreach (SliceHash rightSliceHash, segment.getRightSlices()) {

			_rightSliceToSegments[rightSliceHash].push_back(segmentHash);
			_slices.insert(rightSliceHash);

			if (segment.getSection() + 1 == lastSection)
				_lastSlices.insert(rightSliceHash);
		}

		if (segment.getType() == EndSegmentType)
			numEnds++;
		if (segment.getType() == ContinuationSegmentType)
			numContinuations++;
		if (segment.getType() == BranchSegmentType)
			numBranches++;
	}

	LOG_DEBUG(solutionguarantorlog)
			<< "got " << numEnds << " end segments, "
			<< numContinuations << " continuation segments, and "
			<< numBranches << " branches" << std::endl;

	LOG_ALL(solutionguarantorlog)
			<< "first slices are:" << std::endl;
	foreach (const SliceHash& sliceHash, _firstSlices)
		LOG_ALL(solutionguarantorlog) << sliceHash << " ";
	LOG_ALL(solutionguarantorlog) << std::endl;

	LOG_ALL(solutionguarantorlog)
			<< "last slices are:" << std::endl;
	foreach (const SliceHash& sliceHash, _lastSlices)
		LOG_ALL(solutionguarantorlog) << sliceHash << " ";
	LOG_ALL(solutionguarantorlog) << std::endl;

	// create linear constraints on the variables

	boost::shared_ptr<LinearConstraints> constraints = createConstraints(segments, conflictSets, explicitConstraints);

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
		const ConflictSets&        conflictSets,
		const SegmentConstraints&  explicitConstraints) {

	LOG_DEBUG(solutionguarantorlog) << "creating constraints" << std::endl;

	boost::shared_ptr<LinearConstraints> constraints = boost::make_shared<LinearConstraints>();

	addOverlapConstraints(segments, conflictSets, *constraints);
	addContinuationConstraints(segments, *constraints);
	addExplicitConstraints(explicitConstraints, *constraints);

	return constraints;
}

boost::shared_ptr<LinearObjective>
SolutionGuarantor::createObjective(const SegmentDescriptions& segments) {

	LOG_DEBUG(solutionguarantorlog) << "creating objective for " << segments.size() << " segments" << std::endl;

	boost::shared_ptr<LinearObjective> objective = boost::make_shared<LinearObjective>(segments.size());

	if (segments.size() == 0)
		return objective;

	unsigned int numFeatures = segments.begin()->getFeatures().size();

	if (numFeatures != _weights.size())
		UTIL_THROW_EXCEPTION(
				UsageError,
				"number of features " << numFeatures << " does not match number of weights " << _weights.size());

	foreach (const SegmentDescription& segment, segments) {

		unsigned int var  = _hashToVariable[segment.getHash()];
		double       cost = getCost(segment.getFeatures());

		objective->setCoefficient(var, cost);
	}

	return objective;
}

void
SolutionGuarantor::addOverlapConstraints(
		const SegmentDescriptions& /*segments*/,
		const ConflictSets&        conflictSets,
		LinearConstraints&         constraints) {

	LOG_DEBUG(solutionguarantorlog)
			<< "creating overlap constraints for " << conflictSets.size()
			<< " conflict sets" << std::endl;

	std::set<SliceHash> noConflictSlices = _slices;

	// for each conflict set:
	foreach (const ConflictSet& conflictSet, conflictSets) {

		LinearConstraint constraint;

		// get all segments that use the conflict slices on one side and require 
		// the sum of their variables to be at most one
		foreach (const SliceHash& sliceHash, conflictSet.getSlices()) {

			std::map<SliceHash, std::vector<SegmentHash> >* sliceToSegments = &_leftSliceToSegments;

			// find segments that use the slice on their left side, except if 
			// this is a slice in the last section -- in this case, find 
			// segments that use it on their right side
			if (_lastSlices.count(sliceHash))
				sliceToSegments = &_rightSliceToSegments;

			foreach (const SegmentHash& segmentHash, (*sliceToSegments)[sliceHash]) {

				unsigned int var = _hashToVariable[segmentHash];

				constraint.setCoefficient(var, 1.0);
			}

			noConflictSlices.erase(sliceHash);
		}

		constraint.setRelation(LessEqual);
		constraint.setValue(1.0);

		constraints.add(constraint);
	}

	// Create an exclusivity constraint for the segments one one side of each
	// slice not in any conflict set.
	foreach (const SliceHash& sliceHash, noConflictSlices) {

		LinearConstraint constraint;

		std::map<SliceHash, std::vector<SegmentHash> >* sliceToSegments = &_leftSliceToSegments;

		// find segments that use the slice on their left side, except if
		// this is a slice in the last section -- in this case, find
		// segments that use it on their right side
		if (_lastSlices.count(sliceHash))
			sliceToSegments = &_rightSliceToSegments;

		foreach (const SegmentHash& segmentHash, (*sliceToSegments)[sliceHash])
			constraint.setCoefficient(_hashToVariable[segmentHash], 1.0);

		constraint.setRelation(LessEqual);
		constraint.setValue(1.0);

		constraints.add(constraint);
	}
}

void
SolutionGuarantor::addContinuationConstraints(
		const SegmentDescriptions& /*segments*/,
		LinearConstraints&         constraints) {

	// for each slice
	foreach (const SliceHash& sliceHash, _slices) {

		// if not a first or last slice
		if (_firstSlices.count(sliceHash) || _lastSlices.count(sliceHash))
			continue;

		// get all segments that use this slice from the left
		const std::vector<SegmentHash>& leftSegments = _rightSliceToSegments[sliceHash];

		// get all segments that use this slice from the right
		const std::vector<SegmentHash>& rightSegments = _leftSliceToSegments[sliceHash];

		// require the sum of their variables to be equal

		LinearConstraint constraint;

		LOG_ALL(solutionguarantorlog) << "create new continuation constraint for slice " << sliceHash << std::endl;

		foreach (SegmentHash segmentHash, leftSegments) {
			constraint.setCoefficient(_hashToVariable[segmentHash], 1.0);
			LOG_ALL(solutionguarantorlog) << _hashToVariable[segmentHash] << " is left segment" << std::endl;
		}
		foreach (SegmentHash segmentHash, rightSegments) {
			constraint.setCoefficient(_hashToVariable[segmentHash], -1.0);
			LOG_ALL(solutionguarantorlog) << _hashToVariable[segmentHash] << " is right segment" << std::endl;
		}

		constraint.setRelation(Equal);
		constraint.setValue(0.0);

		constraints.add(constraint);
	}
}

void
SolutionGuarantor::addExplicitConstraints(
		const SegmentConstraints&  explicitConstraints,
		LinearConstraints&         constraints) {
	foreach (const SegmentConstraint& segmentConstraint, explicitConstraints) {
		LinearConstraint constraint;

		foreach (SegmentHash segmentHash, segmentConstraint)
			constraint.setCoefficient(_hashToVariable[segmentHash], 1.0);

		constraint.setRelation(GreaterEqual);
		constraint.setValue(1.0);

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
