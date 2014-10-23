#ifndef LOCAL_SEGMENT_STORE_H__
#define LOCAL_SEGMENT_STORE_H__

#include <boost/shared_ptr.hpp>
#include <catmaid/persistence/SegmentStore.h>
#include <catmaid/blocks/Block.h>
#include <catmaid/blocks/Core.h>
#include <catmaid/blocks/Blocks.h>

class LocalSegmentStore : public SegmentStore {

public:

	LocalSegmentStore() {}

	/**
	 * Associate a set of segment descritptions to a block. A "descritption" is 
	 * a SegmentDescritption that represents a segment only by its hash, 
	 * features, and slice hashes.
	 *
	 * @param segments
	 *              A description of the segments that are supposed to be stored 
	 *              in the database.
	 * @param block
	 *              The block to which to associate the segments.
	 */
	void associateSegmentsToBlock(
			const SegmentDescriptions& segments,
			const Block&               block) {}

	/**
	 * Get a description of all the segments in the given blocks. A 
	 * "descritption" is a SegmentDescritption that represents a segment only by 
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
	boost::shared_ptr<SegmentDescriptions> getSegmentsByBlocks(
			const Blocks& blocks,
			Blocks&       missingBlocks) {}

	/**
	 * Get additional constraints for segments in the given blocks. Typically
	 * these would be user corrections to previous solutions or constraints
	 * inferred from prior tracing.
	 *
	 * @param blocks
	 *              The blocks from which to retrieve the constraints.
	 */
	boost::shared_ptr<SegmentConstraints> getConstraintsByBlocks(
			const Blocks& blocks) {}

	/**
	 * Get weights for Segment features used computing problem cost.
	 */
	std::vector<double> getFeatureWeights() {}

	/**
	 * Store the solution of processing a core.
	 *
	 * @param segmentHashes
	 *              A list of segment hashes that are part of the solution. A 
	 *              concrete implementation has to make sure that all other 
	 *              segments associated to this core are marked as not belonging 
	 *              to the solution.
	 * @param core
	 *              The core for which the solution was generated.
	 */
	void storeSolution(const std::vector<std::size_t>& segmentHashes, const Core& core) {}

	/**
	 * Check whether the segments for the given block have already been 
	 * extracted.
	 */
	bool getSegmentsFlag(const Block& block) {}
};

#endif //LOCAL_SEGMENT_STORE_H__
