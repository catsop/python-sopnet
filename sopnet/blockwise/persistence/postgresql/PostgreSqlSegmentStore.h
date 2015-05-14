#ifndef POSTGRESQL_SEGMENT_STORE_H__
#define POSTGRESQL_SEGMENT_STORE_H__

#include "config.h"
#ifdef HAVE_PostgreSQL

#include <libpq-fe.h>

#include <blockwise/ProjectConfiguration.h>
#include <blockwise/blocks/Blocks.h>
#include <blockwise/blocks/BlockUtils.h>
#include <blockwise/persistence/SegmentDescription.h>
#include <blockwise/persistence/SegmentStore.h>

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
			Blocks&       missingBlocks,
			bool          readCosts);

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
	 * Retrieve a solution as a list of sets of segment hashes for the given 
	 * cores.
	 *
	 * @param cores
	 *              The cores for which to retrieve the solution.
	 */
	std::vector<std::set<SegmentHash> > getSolutionByCores(const Cores& cores);

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
