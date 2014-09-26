#ifndef POSTGRESQL_SLICE_STORE_H__
#define POSTGRESQL_SLICE_STORE_H__

#include "config.h"
#ifdef HAVE_PostgreSQL

#include <iostream>
#include <map>
#include <string>

#include <boost/property_tree/ptree.hpp>
#include <boost/unordered_map.hpp>
#include <catmaid/blocks/Blocks.h>
#include <catmaid/persistence/SlicePointerHash.h>
#include <catmaid/persistence/SliceStore.h>
#include <catmaid/persistence/django/DjangoBlockManager.h>
#include <libpq-fe.h>
#include <pipeline/all.h>
#include <pipeline/all.h>
#include <sopnet/slices/ConflictSets.h>
#include <sopnet/slices/Slices.h>

struct PostgreSqlException : virtual Exception {};

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

	void associate(boost::shared_ptr<Slices> slices, boost::shared_ptr<Block> block);

    boost::shared_ptr<Slices> retrieveSlices(const Blocks& blocks);

	boost::shared_ptr<Blocks> getAssociatedBlocks(boost::shared_ptr<Slice> slice);
	
	void storeConflict(boost::shared_ptr<ConflictSets> conflictSets);
	
	boost::shared_ptr<ConflictSets> retrieveConflictSets(const Slices& slices);

	boost::shared_ptr<DjangoBlockManager> getDjangoBlockManager() const;
	
	void dumpStore();
	
	std::string getHash(const Slice& slice);
	
	/**
	 * Get a cached Slice given its postgreSql hash. This function is intended for use by other
	 * PostgreSql-backed stores. The returned slice may be null even if the hash exists in the 
	 * postgreSql db, if it has not been returned by a previous call to retrieveSlices.
	 */
	boost::shared_ptr<Slice> sliceByHash(const std::string& hash);
	
private:
	void putSlice(boost::shared_ptr<Slice> slice, const std::string hash);

	/**
	 * Stores the connected component that constitutes a slice as a pixel list 
	 * in a local file.
	 */
	void saveConnectedComponent(std::string sliceHash, const ConnectedComponent& component);

	/**
	 * Reads a connected component from a pixel list file, given the slice hash.
	 */
	boost::shared_ptr<ConnectedComponent> readConnectedComponent(std::string sliceHash);
	
	boost::shared_ptr<Slice> ptreeToSlice(const boost::property_tree::ptree& pt);
	boost::shared_ptr<ConflictSet> ptreeToConflictSet(const boost::property_tree::ptree& pt);
	
	std::string generateSliceHash(const Slice& slice);
	
	const std::string _server;
	const int _stack, _project;
	
	boost::shared_ptr<DjangoBlockManager> _blockManager;

	// directory to store the pixel lists of slices
	const std::string _componentDirectory;

	boost::unordered_map<std::string, boost::shared_ptr<Slice> > _hashSliceMap;
	boost::unordered_map<Slice, std::string> _sliceHashMap;
	std::map<unsigned int, boost::shared_ptr<Slice> > _idSliceMap;

	// database connection
	PGconn* _pgConnection;
};

#endif // HAVE_PostgreSQL

#endif //POSTGRESQL_SLICE_STORE_H__
