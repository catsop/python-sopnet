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


/**
 * A SliceStore backed by a JSON interface via django, ie, CATMAID-style. This SliceStore
 * requires/assumes that the Blocks used to store and retrieve data are consistent with those
 * already in the database, ie, as returned by a DjangoBlockManager.
 */
class DjangoSliceStore : public SliceStore
{
public:
	/**
	 * Create a DjangoSliceStore for the given server (which may include port number, as in
	 * 'catmaid:8000'), stack id and project id.
	 */
	DjangoSliceStore(const boost::shared_ptr<DjangoBlockManager> blockManager);
	
	void associate(pipeline::Value<Slices> slices, pipeline::Value<Block> block);

    pipeline::Value<Slices> retrieveSlices(pipeline::Value<Blocks> blocks);

	pipeline::Value<Blocks> getAssociatedBlocks(pipeline::Value<Slice> slice);
	
	void storeConflict(pipeline::Value<ConflictSets> conflictSets);
	
	pipeline::Value<ConflictSets> retrieveConflictSets(pipeline::Value<Slices> slices);

	void dumpStore();
private:
	void putSlice(const boost::shared_ptr<Slice> slice, const std::string hash);
	void appendProjectAndStack(std::ostringstream& os);
	void appendGeometry(const boost::shared_ptr<ConnectedComponent> component,
						std::ostringstream& osX, std::ostringstream& osY);
	
	boost::shared_ptr<Slice> ptreeToSlice(const boost::property_tree::ptree& pt);
	boost::shared_ptr<ConflictSet> ptreeToConflictSet(const boost::property_tree::ptree& pt);
	
	std::string generateSliceHash(const Slice& slice);
	std::string getHash(const Slice& slice);
	
	const std::string _server;
	const int _stack, _project;
	
	boost::shared_ptr<DjangoBlockManager> _blockManager;
	boost::unordered_map<std::string, boost::shared_ptr<Slice> > _hashSliceMap;
	boost::unordered_map<Slice, std::string> _sliceHashMap;
	std::map<unsigned int> _idSliceMap;
};

#endif //DJANGO_SLICE_STORE_H__