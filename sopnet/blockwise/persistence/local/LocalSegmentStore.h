#ifndef LOCAL_SEGMENT_STORE_H__
#define LOCAL_SEGMENT_STORE_H__

#include <boost/shared_ptr.hpp>
#include <blockwise/ProjectConfiguration.h>
#include <blockwise/persistence/SegmentStore.h>
#include <blockwise/blocks/Block.h>
#include <blockwise/blocks/Core.h>
#include <blockwise/blocks/Blocks.h>

class LocalSegmentStore : public SegmentStore {

public:

	LocalSegmentStore(const ProjectConfiguration& config);

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
			const Block&               block);

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
			Blocks&       missingBlocks,
			bool          readCosts);

	/**
	 * Get additional constraints for segments in the given blocks. Typically
	 * these would be user corrections to previous solutions or constraints
	 * inferred from prior tracing.
	 *
	 * NOTE: LocalSegmentStore always returns no constraints.
	 *
	 * @param blocks
	 *              The blocks from which to retrieve the constraints.
	 */
	boost::shared_ptr<SegmentConstraints> getConstraintsByBlocks(
			const Blocks& blocks);

	/**
	 * Get weights for Segment features used computing problem cost.
	 */
	std::vector<double> getFeatureWeights();

	/**
	 * Store costs for segments.
	 */
	void storeSegmentCosts(const std::map<SegmentHash, double>& costs);

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
	void storeSolution(const std::vector<std::set<SegmentHash> >& assemblies, const Core& core);


	/**
	 * Check whether the segments for the given block have already been 
	 * extracted.
	 */
	bool getSegmentsFlag(const Block& block);

private:

	std::map<Block, SegmentDescriptions> _segments;

	const std::vector<double> _weights;

	std::map<Core, std::vector<std::set<SegmentHash> > > _solutions;
};

#endif //LOCAL_SEGMENT_STORE_H__
