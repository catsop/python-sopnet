#ifndef SOLUTION_GUARANTOR_H__
#define SOLUTION_GUARANTOR_H__

#include <boost/shared_ptr.hpp>

#include <blockwise/ProjectConfiguration.h>
#include <blockwise/persistence/SegmentStore.h>
#include <blockwise/persistence/SliceStore.h>
#include <blockwise/blocks/BlockUtils.h>
#include <blockwise/blocks/Core.h>

#include <segments/SegmentHash.h>
#include <solvers/LinearConstraints.h>
#include <solvers/LinearObjective.h>

class SolutionGuarantor {

public:

	/**
	 * Create a new SolutionGuarantor using the given database stores.
	 *
	 * @param projectConfiguration
	 *              The ProjectConfiguration used to configure Block and Core
	 *              parameters.
	 *
	 * @param segmentStore
	 *              The SegmentStore to use to retrieve segments and store the  
	 *              solution.
	 *
	 * @param sliceStore
	 *              The slice store to retrieve conflict sets on slices.
	 *
	 * @param corePadding
	 *              The number of blocks to pad around a core in order to 
	 *              eliminate border effects. The solution will be computed on 
	 *              the padded core, but only the solution of the core will be 
	 *              stored.
	 *
	 * @param forceExplanation
	 *              If true, exactly one member of each conflict set must be in
	 *              the solution.
	 *
	 * @param readCosts
	 *              If true, cached costs available from the SegmentStore are
	 *              used instead of features and weights where available.
	 *
	 * @param storeCosts
	 *              If true, computed segment costs are saved to SegmentStore.
	 */
	SolutionGuarantor(
			const ProjectConfiguration&     projectConfiguration,
			boost::shared_ptr<SegmentStore> segmentStore,
			boost::shared_ptr<SliceStore>   sliceStore,
			unsigned int                    corePadding,
			bool                            forceExplanation,
			bool                            readCosts,
			bool                            storeCosts);

	/**
	 * Get the solution for the given cores.
	 *
	 * @param core
	 *              The core to compute the solution for.
	 */
	Blocks guaranteeSolution(const Core& core);

protected:

	std::vector<std::set<SegmentHash> > extractAssemblies(
			const std::vector<SegmentHash>& solution,
			const SegmentDescriptions& segments);

private:

	// get all blocks of the padded core
	Blocks getPaddedCoreBlocks(const Core& core);

	std::vector<SegmentHash> computeSolution(
			const SegmentDescriptions& segments,
			const ConflictSets&        conflictSets,
			const SegmentConstraints&  explicitConstraints);

	boost::shared_ptr<LinearConstraints> createConstraints(
			const SegmentDescriptions& segments,
			const ConflictSets&        conflictSets,
			const SegmentConstraints&  explicitConstraints);

	boost::shared_ptr<LinearObjective> createObjective(const SegmentDescriptions& segments);

	void addOverlapConstraints(
			const SegmentDescriptions& segments,
			const ConflictSets&        conflictSets,
			LinearConstraints&         constraints);

	void addContinuationConstraints(
			const SegmentDescriptions& segments,
			LinearConstraints&         constraints);

	void addExplicitConstraints(
			const SegmentConstraints&  explicitConstraints,
			LinearConstraints&         constraints);

	double getCost(const std::vector<double>& features);

	util::box<unsigned int, 3> segmentsBoundingBox(const SegmentDescriptions& segments);

	std::vector<SegmentHash> cullSolutionToCore(
			const std::vector<SegmentHash>& solutions,
			const SegmentDescriptions& segments,
			const Core& core);

	boost::shared_ptr<SegmentStore> _segmentStore;
	boost::shared_ptr<SliceStore>   _sliceStore;

	unsigned int _corePadding;

	bool _forceExplanation;
	bool _readCosts;
	bool _storeCosts;

	// mappings from segment hashes to their variable number in the ILP
	std::map<SegmentHash, unsigned int> _hashToVariable;
	std::map<unsigned int, SegmentHash> _variableToHash;

	// mappings from slice hashes to hashes of segments that use the slice 
	// either on the left or right side
	std::map<SliceHash, std::vector<SegmentHash> > _leftSliceToSegments;
	std::map<SliceHash, std::vector<SegmentHash> > _rightSliceToSegments;

	// set of all slices
	std::set<SliceHash> _slices;

	// set of all slices not associated to the request blocks
	std::set<SliceHash> _leafSlices;

	// the feature weights
	std::vector<double> _weights;

	BlockUtils _blockUtils;
};

#endif //SOLUTION_GUARANTOR_H__
