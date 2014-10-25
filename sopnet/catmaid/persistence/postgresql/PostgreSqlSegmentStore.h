#ifndef POSTGRESQL_SEGMENT_STORE_H__
#define POSTGRESQL_SEGMENT_STORE_H__

#include "config.h"
#ifdef HAVE_PostgreSQL

#include <libpq-fe.h>

#include <catmaid/ProjectConfiguration.h>
#include <catmaid/blocks/Blocks.h>
#include <catmaid/blocks/BlockUtils.h>
#include <catmaid/persistence/SegmentDescription.h>
#include <catmaid/persistence/SegmentStore.h>

class PostgreSqlSegmentStore : public SegmentStore {

public:
	/**
	 * Create a PostgreSqlSegmentStore over the same parameters given to the
	 * DjangoBlockManager here.
	 *
	 * @param config
	 *             The project configuration with all required information.
	 */
	PostgreSqlSegmentStore(const ProjectConfiguration& config);

	~PostgreSqlSegmentStore();

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
	void associateSegmentsToBlock(
			const SegmentDescriptions& segments,
			const Block&               block);

	/**
	 * Get a description of all the segments in the given blocks. A
	 * "descritption" is a SegmentDescription that represents a segment only by
	 * its hash, features, and slice hashes.
	 *
	 * @param blocks
	 *              The blocks from which to retrieve the segments.
	 */
	boost::shared_ptr<SegmentDescriptions> getSegmentsByBlocks(
			const Blocks& blocks,
			Blocks&       missingBlocks);

	/**
	 * Get additional constraints for segments in the given blocks. Typically
	 * these would be user corrections to previous solutions or constraints
	 * inferred from prior tracing.
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
	void storeSolution(const std::vector<SegmentHash>& segmentHashes, const Core& core);

	/**
	 * Check whether the segments for the given block have already been 
	 * extracted.
	 */
	bool getSegmentsFlag(const Block& block);

private:
	// general configuration
	const ProjectConfiguration& _config;

	// database connection
	PGconn* _pgConnection;
};

#endif //HAVE_PostgreSQL

#endif //POSTGRESQL_SEGMENT_STORE_H__
