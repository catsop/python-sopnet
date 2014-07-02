#ifndef DJANGO_SLICE_STORE_H__
#define DJANGO_SLICE_STORE_H__

#include <catmaid/persistence/SliceStore.h>
#include <catmaid/django/DjangoBlockManager.h>
#include <pipeline/all.h>
#include <sopnet/slices/Slices.h>
#include <sopnet/block/Blocks.h>
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
class DjangoSliceStore : public SliceStore
{
public:
	/**
	 * Create a DjangoSliceStore over the same parameters given to the DjangoBlockManager here.
	 */
	DjangoSliceStore(const boost::shared_ptr<DjangoBlockManager> blockManager);
	
	void associate(pipeline::Value<Slices> slices, pipeline::Value<Block> block);

    pipeline::Value<Slices> retrieveSlices(const Blocks& blocks);

	pipeline::Value<Blocks> getAssociatedBlocks(pipeline::Value<Slice> slice);
	
	void storeConflict(pipeline::Value<ConflictSets> conflictSets);
	
	pipeline::Value<ConflictSets> retrieveConflictSets(pipeline::Value<Slices> slices);

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
	void appendGeometry(const boost::shared_ptr<ConnectedComponent> component,
						std::ostringstream& osX, std::ostringstream& osY);
	
	boost::shared_ptr<Slice> ptreeToSlice(const boost::property_tree::ptree& pt);
	boost::shared_ptr<ConflictSet> ptreeToConflictSet(const boost::property_tree::ptree& pt);
	
	std::string generateSliceHash(const Slice& slice);
	
	const std::string _server;
	const int _stack, _project;
	
	boost::shared_ptr<DjangoBlockManager> _blockManager;
	boost::unordered_map<std::string, boost::shared_ptr<Slice> > _hashSliceMap;
	boost::unordered_map<Slice, std::string> _sliceHashMap;
	std::map<unsigned int, boost::shared_ptr<Slice> > _idSliceMap;
};

#endif //DJANGO_SLICE_STORE_H__
