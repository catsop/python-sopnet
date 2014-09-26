#ifndef DJANGO_SLICE_STORE_H__
#define DJANGO_SLICE_STORE_H__

#include "DjangoBlockManager.h"
#include <catmaid/persistence/SliceStore.h>
#include <pipeline/all.h>
#include <sopnet/slices/Slices.h>
#include <catmaid/blocks/Blocks.h>
#include <sopnet/slices/ConflictSets.h>
#include <boost/unordered_map.hpp>
#include <catmaid/persistence/SlicePointerHash.h>
#include <iostream>
#include <map>
#include <boost/property_tree/ptree.hpp>
#include <pipeline/all.h>


/**
 * A SliceStore backed by a JSON interface via django, ie, CATMAID-style. This SliceStore
 * requires/assumes that the Blocks used to store and retrieve data are consistent with those
 * already in the database, ie, as returned by a DjangoBlockManager.
 */
class DjangoSliceStore : public SliceStore {

public:

	/**
	 * Create a DjangoSliceStore over the same parameters given to the 
	 * DjangoBlockManager here.
	 *
	 * @param blockManager
	 *             The block manager to use.
	 * @param componentDirectory
	 *             A directory to use for storing the pixel lists of slices.
	 */
	DjangoSliceStore(const boost::shared_ptr<DjangoBlockManager> blockManager, const std::string& componentDirectory = "/tmp");

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
	boost::shared_ptr<Slices> getSlicesByBlocks(const Blocks& blocks) {}

	/**
	 * Get all the conflict sets that are associated to the given blocks. The 
	 * conflict sets will contain the hashes of slices.
	 */
	boost::shared_ptr<ConflictSets> getConflictSetsByBlocks(const Blocks& block) {}


	/******************************************
	 * OLD INTERFACE DEFINITION -- DEPRECATED *
	 ******************************************/
	
	void associate(boost::shared_ptr<Slices> slices, boost::shared_ptr<Block> block);

    boost::shared_ptr<Slices> retrieveSlices(const Blocks& blocks);

	boost::shared_ptr<Blocks> getAssociatedBlocks(boost::shared_ptr<Slice> slice);
	
	void storeConflict(boost::shared_ptr<ConflictSets> conflictSets);
	
	boost::shared_ptr<ConflictSets> retrieveConflictSets(const Slices& slices);

	boost::shared_ptr<DjangoBlockManager> getDjangoBlockManager() const;
	
	void dumpStore();
	
	std::string getHash(const Slice& slice);
	
	/**
	 * Get a cached Slice given its django hash. This function is intended for use by other
	 * Django-backed stores. The returned slice may be null even if the hash exists in the 
	 * django db, if it has not been returned by a previous call to retrieveSlices.
	 */
	boost::shared_ptr<Slice> sliceByHash(const std::string& hash);
	
private:
	void putSlice(boost::shared_ptr<Slice> slice, const std::string hash);
	
	void appendProjectAndStack(std::ostringstream& os);

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
};

#endif //DJANGO_SLICE_STORE_H__
