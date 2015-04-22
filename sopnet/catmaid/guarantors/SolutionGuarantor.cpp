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
		bool                            forceExplanation,
		bool                            readCosts,
		bool                            storeCosts) :
	_segmentStore(segmentStore),
	_sliceStore(sliceStore),
	_corePadding(corePadding),
	_forceExplanation(forceExplanation),
	_readCosts(readCosts),
	_storeCosts(storeCosts),
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
	boost::shared_ptr<SegmentDescriptions> segments = _segmentStore->getSegmentsByBlocks(blocks, missingBlocks, _readCosts);

	if (!missingBlocks.empty())
		return missingBlocks;

	// get all conflict sets for blocks overlapping segments
	Blocks expandedBlocks = _blockUtils.getBlocksInBox(segmentsBoundingBox(*segments));
	LOG_DEBUG(solutionguarantorlog) << "Expanded blocks for conflict sets are " << expandedBlocks << "." << std::endl;
	boost::shared_ptr<ConflictSets> conflictSets = _sliceStore->getConflictSetsByBlocks(expandedBlocks, missingBlocks);

	if (!missingBlocks.empty())
		return missingBlocks;

	boost::shared_ptr<SegmentConstraints> explicitConstraints = _segmentStore->getConstraintsByBlocks(blocks);

	LOG_DEBUG(solutionguarantorlog) << "computing solution..." << std::endl;

	_weights = _segmentStore->getFeatureWeights();

	// compute solution
	std::vector<SegmentHash> solution = computeSolution(*segments, *conflictSets, *explicitConstraints);

	LOG_DEBUG(solutionguarantorlog) << "solution contains " << solution.size() << " segments" << std::endl;

	// cull solution to requested core
	std::vector<SegmentHash> culledSolution = cullSolutionToCore(solution, *segments, core);

	// extract assemblies
	std::vector<std::set<SegmentHash> > assemblies = extractAssemblies(culledSolution, *segments);

	// store solution
	_segmentStore->storeSolution(assemblies, core);

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

		const unsigned int segmentSection = segment.getSection();
		// Special case for the first section in a stack to prevent underflow
		firstSection = std::min(firstSection,
				segmentSection == 0 ? segmentSection : segmentSection - 1);
		lastSection  = std::max(lastSection,  segmentSection);
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
	_variableToHash.clear();

	std::set<SliceHash> hasEnd;

	unsigned int nextVar = 0;
	foreach (const SegmentDescription& segment, segments) {

		SegmentHash segmentHash = segment.getHash();

		_hashToVariable[segmentHash] = nextVar;
		_variableToHash[nextVar] = segmentHash;
		nextVar++;

		foreach (SliceHash leftSliceHash, segment.getLeftSlices()) {

			_leftSliceToSegments[leftSliceHash].push_back(segmentHash);
			_slices.insert(leftSliceHash);

			if (segment.getType() == EndSegmentType) hasEnd.insert(leftSliceHash);
		}

		foreach (SliceHash rightSliceHash, segment.getRightSlices()) {

			_rightSliceToSegments[rightSliceHash].push_back(segmentHash);
			_slices.insert(rightSliceHash);

			if (segment.getType() == EndSegmentType) hasEnd.insert(rightSliceHash);
		}

		if (segment.getType() == EndSegmentType)
			numEnds++;
		if (segment.getType() == ContinuationSegmentType)
			numContinuations++;
		if (segment.getType() == BranchSegmentType)
			numBranches++;
	}

	// Leaf slices are slices without an end segment.
	std::set_difference(_slices.begin(), _slices.end(), hasEnd.begin(), hasEnd.end(),
			std::inserter(_leafSlices, _leafSlices.end()));

	LOG_DEBUG(solutionguarantorlog)
			<< "got " << numEnds << " end segments, "
			<< numContinuations << " continuation segments, and "
			<< numBranches << " branches" << std::endl;

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

	// newly computed segment costs
	std::map<SegmentHash, double> newCosts;

	foreach (const SegmentDescription& segment, segments) {

		unsigned int var  = _hashToVariable[segment.getHash()];
		double       cost = segment.getCost();

		if (!_readCosts || std::isnan(cost)) {

			cost = getCost(segment.getFeatures());
			newCosts[segment.getHash()] = cost;
		}

		objective->setCoefficient(var, cost);
	}

	// Store costs if requested.
	if (_storeCosts && !newCosts.empty()) _segmentStore->storeSegmentCosts(newCosts);

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
		bool anySet = false;
		bool anyLeaf = false;

		// get all segments that use the conflict slices on one side and require 
		// the sum of their variables to be at most one
		foreach (const SliceHash& sliceHash, conflictSet.getSlices()) {

			// Because conflict sets from expanded blocks may include slices not
			// in this set of segments, first check for the slice.
			if (_slices.count(sliceHash)) {

				anySet = true;

				std::map<SliceHash, std::vector<SegmentHash> >* sliceToSegments = &_leftSliceToSegments;

				if (_leafSlices.count(sliceHash)) anyLeaf = true;

				// Find segments that use the slice on their left side, except
				// if this is a slice in the last section or a leaf slice with
				// no left segments. In this case, find segments that use it on
				// their right side.
				if (0 == sliceToSegments->count(sliceHash))
					sliceToSegments = &_rightSliceToSegments;

				foreach (const SegmentHash& segmentHash, (*sliceToSegments)[sliceHash]) {

					unsigned int var = _hashToVariable[segmentHash];

					constraint.setCoefficient(var, 1.0);
				}
			}

			noConflictSlices.erase(sliceHash);
		}

		// Do not force explanation if this conflict set involves a leaf slice.
		constraint.setRelation(_forceExplanation && !anyLeaf && conflictSet.isMaximalClique() ? Equal : LessEqual);
		constraint.setValue(1.0);

		// Only add the constraint if any variables were set. This is necessary
		// since some expanded block conflict sets do not involve any _slices.
		if (anySet) constraints.add(constraint);
	}

	// Create an exclusivity constraint for the segments one one side of each
	// slice not in any conflict set.
	foreach (const SliceHash& sliceHash, noConflictSlices) {

		LinearConstraint constraint;

		std::map<SliceHash, std::vector<SegmentHash> >* sliceToSegments = &_leftSliceToSegments;

		// Find segments that use the slice on their left side, except
		// if this is a slice in the last section or a leaf slice with
		// no left segments. In this case, find segments that use it on
		// their right side.
		if (0 == sliceToSegments->count(sliceHash))
			sliceToSegments = &_rightSliceToSegments;

		foreach (const SegmentHash& segmentHash, (*sliceToSegments)[sliceHash])
			constraint.setCoefficient(_hashToVariable[segmentHash], 1.0);

		// Do not force explanation if this is a leaf slice.
		constraint.setRelation(_forceExplanation && 0 == _leafSlices.count(sliceHash) ? Equal : LessEqual);
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

		// If the slice has no segments on one side (some leaf, first, last
		// slices), ignore it.
		if (0 == _rightSliceToSegments.count(sliceHash) ||
			0 == _leftSliceToSegments.count(sliceHash))
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

	typedef std::map<SegmentHash, double>::value_type segmentCoeff;
	foreach (const SegmentConstraint& segmentConstraint, explicitConstraints) {
		LinearConstraint constraint;

		foreach (const segmentCoeff& coeff, segmentConstraint.getCoefficients())
			constraint.setCoefficient(_hashToVariable[coeff.first], coeff.second);

		constraint.setRelation(segmentConstraint.getRelation());
		constraint.setValue(segmentConstraint.getValue());

		constraints.add(constraint);
	}
}

double
SolutionGuarantor::getCost(const std::vector<double>& features) {

	if (features.size() != _weights.size())
		UTIL_THROW_EXCEPTION(
				UsageError,
				"number of features " << features.size() << " does not match number of weights " << _weights.size());

	double cost = 0;

	std::vector<double>::const_iterator i = features.begin();
	std::vector<double>::const_iterator j = _weights.begin();

	while (i != features.end()) {

		cost += (*i)*(*j);
		++i;
		++j;
	}

	return cost;
}

util::box<unsigned int>
SolutionGuarantor::segmentsBoundingBox(const SegmentDescriptions& segments)
{
	if (segments.size() == 0)
		return util::box<unsigned int>(0, 0, 0, 0, 0, 0);

	util::rect<unsigned int> bound(0, 0, 0, 0);
	unsigned int zMax = 0;
	unsigned int zMin = 0;

	foreach (const SegmentDescription& segment, segments) {

		if (bound.area() == 0) {

			bound = segment.get2DBoundingBox();
			// Because bounding box max is strictly greater than contents but
			// segment includes a slice in its section supremum, zMax must be
			// one greater than section supremum.
			zMax  = segment.getSection() + 1;
			zMin  = segment.getSection() == 0 ? segment.getSection() : segment.getSection() - 1;

		} else {

			bound.fit(segment.get2DBoundingBox());
			zMax = std::max(zMax, segment.getSection() + 1);
			zMin = std::min(zMin, segment.getSection() == 0 ? segment.getSection() : segment.getSection() - 1);
		}
	}

	return util::box<unsigned int>(bound.minX, bound.minY, zMin, bound.maxX, bound.maxY, zMax);
}

std::vector<SegmentHash>
SolutionGuarantor::cullSolutionToCore(
		const std::vector<SegmentHash>& solution,
		const SegmentDescriptions& segments,
		const Core& core) {

	std::vector<SegmentHash> culledSolution;
	culledSolution.reserve(solution.size());

	std::set<SegmentHash> solutionLookup(solution.begin(), solution.end());

	util::box<unsigned int> coreBoundingBox = _blockUtils.getBoundingBox(core);
	util::rect<unsigned int> coreBoundingRect = coreBoundingBox.project_xy();

	foreach (const SegmentDescription& segment, segments) {

		SegmentHash segmentHash = segment.getHash();

		if (solutionLookup.count(segmentHash)) {

			// test in z
			if (segment.getSection() >= coreBoundingBox.min.z &&
			    segment.getSection() <= coreBoundingBox.max.z &&
			    coreBoundingRect.intersects(segment.get2DBoundingBox())) {

				culledSolution.push_back(segmentHash);
			}
		}
	}

	return culledSolution;
}

std::vector<std::set<SegmentHash> >
SolutionGuarantor::extractAssemblies(
		const std::vector<SegmentHash>& solution,
		const SegmentDescriptions& segments) {

	std::set<SegmentHash> solutionLookup(solution.begin(), solution.end());
	std::set<SliceHash> unvisitedSlices;
	std::stack<SliceHash> boundarySlices;
	typedef std::pair<SliceHash, SegmentHash> adjacencyEdge;
	std::map<SliceHash, std::vector<adjacencyEdge> > sliceNeighbors;
	std::vector<std::set<SegmentHash> > assemblies;

	// Construct slice adjacency graph.
	foreach (const SegmentDescription& segment, segments) {

		if (solutionLookup.count(segment.getHash())) {

			const std::vector<SliceHash>& leftSliceHashes = segment.getLeftSlices();
			const std::vector<SliceHash>& rightSliceHashes = segment.getLeftSlices();
			unvisitedSlices.insert(leftSliceHashes.begin(), leftSliceHashes.end());
			unvisitedSlices.insert(rightSliceHashes.begin(), rightSliceHashes.end());

			foreach (const SliceHash& leftHash, leftSliceHashes) {

				foreach (const SliceHash& rightHash, rightSliceHashes) {

					sliceNeighbors[leftHash].push_back(std::make_pair(rightHash, segment.getHash()));
					sliceNeighbors[rightHash].push_back(std::make_pair(leftHash, segment.getHash()));
				}
			}
		}
	}

	// Collect connected components (assemblies).
	while (!unvisitedSlices.empty()) {

		std::set<SegmentHash> assembly;

		SliceHash seedSlice = *(unvisitedSlices.begin());
		boundarySlices.push(seedSlice);

		while (!boundarySlices.empty()) {

			SliceHash currentSlice = boundarySlices.top();
			boundarySlices.pop();

			unvisitedSlices.erase(currentSlice);

			foreach (const adjacencyEdge& neighbor, sliceNeighbors[currentSlice]) {

				assembly.insert(neighbor.second);

				if (unvisitedSlices.count(neighbor.first))
					boundarySlices.push(neighbor.first);
			}
		}

		assemblies.push_back(assembly);
	}

	return assemblies;
}
