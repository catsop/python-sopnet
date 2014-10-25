#ifndef POSTGRESQL_SLICE_STORE_H__
#define POSTGRESQL_SLICE_STORE_H__

#include "config.h"
#ifdef HAVE_PostgreSQL

#include <catmaid/ProjectConfiguration.h>
#include <catmaid/blocks/Blocks.h>
#include <catmaid/blocks/BlockUtils.h>
#include <catmaid/persistence/SliceStore.h>
#include <sopnet/slices/ConflictSets.h>
#include <sopnet/slices/Slices.h>
#include <libpq-fe.h>

#include "PostgreSqlUtils.h"

/**
 * Implementation of a slice store that directly communicates with the 
 * PostgreSql backend.
 */
class PostgreSqlSliceStore : public SliceStore
{
public:
	/**
	 * Create a PostgreSqlSliceStore over the same parameters given to the 
	 * DjangoBlockManager here.
	 *
	 * @param config
	 *             The project configuration with all required information.
	 */
	PostgreSqlSliceStore(const ProjectConfiguration& config);

	~PostgreSqlSliceStore();

	/**
	 * Associate a set of slices to a block.
	 */
	void associateSlicesToBlock(
			const Slices& slices,
			const Block&  block,
			bool  doneWithBlock = true);

	/**
	 * Associate a set of conflict sets to a block. The conflict sets are 
	 * assumed to hold the hashes of the slices.
	 */
	void associateConflictSetsToBlock(
			const ConflictSets& conflictSets,
			const Block&        block);

	/**
	 * Get all slices that are associated to the given blocks. This creates 
	 * "real" slices in the sense that the geometry of the slices will be 
	 * restored.
	 */
	boost::shared_ptr<Slices> getSlicesByBlocks(
			const Blocks& blocks,
			Blocks&       missingBlocks);

	/**
	 * Get all the conflict sets that are associated to the given blocks. The 
	 * conflict sets will contain the hashes of slices.
	 */
	boost::shared_ptr<ConflictSets> getConflictSetsByBlocks(
			const Blocks& blocks,
			Blocks&       missingBlocks);

	/**
	 * Check whether the slices for the given block have already been extracted.
	 */
	bool getSlicesFlag(const Block& block);

private:

	// general configuration
	const ProjectConfiguration& _config;

	// database connection
	PGconn* _pgConnection;

	/**
	 * Store a connected component as a file in the component directory.
	 */
	void saveConnectedComponent(std::string slicePostgreId, const ConnectedComponent& component);

	/**
	 * Load a connected component from a file in the component directory.
	 */
	boost::shared_ptr<ConnectedComponent> loadConnectedComponent(std::string slicePostgreId);
};

#endif // HAVE_PostgreSQL

#endif //POSTGRESQL_SLICE_STORE_H__
