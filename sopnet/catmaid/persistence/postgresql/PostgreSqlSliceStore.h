#ifndef POSTGRESQL_SLICE_STORE_H__
#define POSTGRESQL_SLICE_STORE_H__

#include "config.h"
#ifdef HAVE_PostgreSQL

#include <catmaid/blocks/Blocks.h>
#include <catmaid/persistence/SliceStore.h>
#include <catmaid/persistence/django/DjangoBlockManager.h>
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
	 * @param blockManager
	 *             The block manager to use.
	 * @param componentDirectory
	 *             A directory to use for storing the pixel lists of slices.
	 */
	PostgreSqlSliceStore(
			const boost::shared_ptr<DjangoBlockManager> blockManager,
			const std::string& componentDirectory = "/tmp",
			const std::string& pgHost = "",
			const std::string& pgUser = "catsop_user",
			const std::string& pgPassword = "catsop_janelia_test",
			const std::string& pgDatabaseName = "catsop");

	~PostgreSqlSliceStore();

	/**
	 * Associate a set of slices to a block.
	 */
	void associateSlicesToBlock(
			const Slices& slices,
			const Block&  block) {}

	/**
	 * Associate a set of conflict sets to a block. The conflict sets are 
	 * assumed to hold the hashes of the slices.
	 */
	void associateConflictSetsToBlock(
			const ConflictSets& conflictSets,
			const Block&        block) {}

	/**
	 * Get all slices that are associated to the given blocks. This creates 
	 * "real" slices in the sense that the geometry of the slices will be 
	 * restored.
	 */
	boost::shared_ptr<Slices> getSlicesByBlocks(
			const Blocks& blocks,
			Blocks&       missingBlocks) {}

	/**
	 * Get all the conflict sets that are associated to the given blocks. The 
	 * conflict sets will contain the hashes of slices.
	 */
	boost::shared_ptr<ConflictSets> getConflictSetsByBlocks(
			const Blocks& block,
			Blocks&       missingBlocks) {}

	// directory to store the pixel lists of slices
	const std::string _componentDirectory;

	// database connection
	PGconn* _pgConnection;
};

#endif // HAVE_PostgreSQL

#endif //POSTGRESQL_SLICE_STORE_H__
