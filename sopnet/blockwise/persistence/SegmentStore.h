#ifndef SEGMENT_STORE_H__
#define SEGMENT_STORE_H__

#include <boost/shared_ptr.hpp>

#include <blockwise/blocks/Block.h>
#include <blockwise/blocks/Blocks.h>
#include <blockwise/blocks/Core.h>
#include <blockwise/blocks/Cores.h>
#include <blockwise/persistence/SegmentConstraints.h>
#include <blockwise/persistence/SegmentDescriptions.h>

/**
 * Segment store interface definition.
 */
class SegmentStore {

public:

	/**
	 * Associate a set of segment descritptions to a block. A "descritption" is 
	 * a SegmentDescription that represents a segment only by its hash, 
	 * features, and slice hashes.
	 *
	 * @param segments
	 *              A description of the segments that are supposed to be stored 
	 *              in the database.
	 * @param block
	 *              The block to which to associate the segments.
	 */
	virtual void associateSegmentsToBlock(
			const SegmentDescriptions& segments,
			const Block&               block) = 0;

	/**
	 * Get a description of all the segments in the given blocks. A 
	 * "descritption" is a SegmentDescription that represents a segment only by 
	 * its hash, features, and slice hashes.
	 *
	 * @param blocks
	 *              The blocks from which to retrieve the segments.
	 *
	 * @param missingBlocks
	 *              A reference to a block collection. This collection will be 
	 *              filled with the blocks for which no segments are available, 
	 *              yet. This collection will be empty on success.
	 */
	virtual boost::shared_ptr<SegmentDescriptions> getSegmentsByBlocks(
			const Blocks& blocks,
			Blocks&       missingBlocks,
			bool          readCosts) = 0;

	/**
	 * Get additional constraints for segments in the given blocks. Typically
	 * these would be user corrections to previous solutions or constraints
	 * inferred from prior tracing.
	 *
	 * @param blocks
	 *              The blocks from which to retrieve the constraints.
	 */
	virtual boost::shared_ptr<SegmentConstraints> getConstraintsByBlocks(
			const Blocks& blocks) = 0;

	/**
	 * Get weights for Segment features used computing problem cost.
	 */
	virtual std::vector<double> getFeatureWeights() = 0;

	/**
	 * Store costs for segments.
	 */
	virtual void storeSegmentCosts(const std::map<SegmentHash, double>& costs) = 0;

	/**
	 * Store the solution of processing a core.
	 *
	 * @param assemblies
	 *              A list of sets of segment hashes that are part of the
	 *              solution. Each set is a connected component of segments (an
	 *              assembly) A concrete implementation has to make sure that
	 *              all other segments associated to this core are marked as
	 *              not belonging to the solution.
	 * @param core
	 *              The core for which the solution was generated.
	 */
	virtual void storeSolution(const std::vector<std::set<SegmentHash> >& assemblies, const Core& core) = 0;

	/**
	 * Retrieve a solution as a list of sets of segment hashes for the given 
	 * cores.
	 *
	 * @param cores
	 *              The cores for which to retrieve the solution.
	 */
	virtual std::vector<std::set<SegmentHash> > getSolutionByCores(const Cores& cores) = 0;

	/**
	 * Check whether the segments for the given block have already been 
	 * extracted.
	 */
	virtual bool getSegmentsFlag(const Block& block) = 0;
};


#endif //SEGMENT_STORE_H__
