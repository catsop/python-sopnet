#include <fstream>
#include "DjangoSliceStore.h"
#include "DjangoUtils.h"
#include <util/httpclient.h>
#include <sopnet/slices/ComponentTreeConverter.h>
#include <imageprocessing/ConnectedComponent.h>
#include <imageprocessing/Image.h>
#include <util/point.hpp>
#include <util/ProgramOptions.h>

logger::LogChannel djangoslicestorelog("djangoslicestorelog", "[DjangoSliceStore] ");


util::ProgramOption optionDjangoSliceStoreNoCache(
util::_module = 			"core",
util::_long_name = 			"djangoSliceStoreNoCache",
util::_description_text = 	"Do not use cache for Django Slice Store",
util::_default_value =		false);


DjangoSliceStore::DjangoSliceStore(
		const boost::shared_ptr<DjangoBlockManager> blockManager,
		const std::string& componentDirectory) : 
	_server(blockManager->getServer()),
	_stack(blockManager->getStack()),
	_project(blockManager->getProject()),
	_blockManager(blockManager),
	_componentDirectory(componentDirectory)
{
}


void
DjangoSliceStore::associate(boost::shared_ptr<Slices> slices, boost::shared_ptr<Block> block)
{
	if (slices->size() == 0)
		return;

	boost::shared_ptr<ptree> insertPt, assocPt;

	{
		int i = 0;
		std::ostringstream insertUrl, insertPostData;

		appendProjectAndStack(insertUrl);
		insertUrl << "/insert_slices";
		
		// -- Step 1 : insert slices into database, if they haven't been already --
		   
		// Form POST data
		insertPostData << "n=" << slices->size();
		foreach (boost::shared_ptr<Slice> slice, *slices)
		{
			// TODO: don't send slices that are already in the db.
			
			std::ostringstream osX, osY;
			std::string hash = getHash(*slice);
			util::point<double> ctr = slice->getComponent()->getCenter();
			
			// Make sure that the slice is in the id slice map
			putSlice(slice, hash);
			
			saveConnectedComponent(hash, *slice->getComponent());

			// Section
			insertPostData << "&section_" << i << "=" << slice->getSection();
			// Hash
			insertPostData << "&hash_" << i << "=" << hash;
			// Centroid
			insertPostData << "&cx_" << i << "=" << ctr.x;
			insertPostData << "&cy_" << i << "=" << ctr.y;
			// Bounding Box
			const util::rect<unsigned int>& bb = slice->getComponent()->getBoundingBox();
			insertPostData << "&minx_" << i << "=" << bb.minX;
			insertPostData << "&maxx_" << i << "=" << bb.maxX;
			insertPostData << "&miny_" << i << "=" << bb.minY;
			insertPostData << "&maxy_" << i << "=" << bb.maxY;
			// Size
			insertPostData << "&size_" << i << "=" << slice->getComponent()->getSize();
			// Value
			insertPostData << "&value_" << i << "=" << slice->getComponent()->getValue();
			
			++i;
		}
		
		insertPt = HttpClient::postPropertyTree(insertUrl.str(), insertPostData.str());

		DjangoUtils::checkDjangoError(insertPt, insertUrl.str());
	}

	{
		// -- Step 2 : associate the slices with the given block --
		std::ostringstream assocUrl, assocPostData;
		std::string delim = "";

		appendProjectAndStack(assocUrl);

		assocUrl << "/slices_block";

		assocPostData << "hash=";
		foreach (boost::shared_ptr<Slice> slice, *slices)
		{
			assocPostData << delim << _sliceHashMap[*slice];
			delim = ",";
		}
		
		assocPostData << "&block=" << block->getId();
		
		assocPt = HttpClient::postPropertyTree(assocUrl.str(), assocPostData.str());

		DjangoUtils::checkDjangoError(assocPt, assocUrl.str());
	}
}

boost::shared_ptr<Slices>
DjangoSliceStore::retrieveSlices(const Blocks& blocks)
{
	std::ostringstream url;
	std::ostringstream post;
	std::string delim = "";
	boost::shared_ptr<ptree> pt;
	boost::shared_ptr<Slices> slices = boost::make_shared<Slices>();
	ptree slicesTree;
	
	appendProjectAndStack(url);
	url << "/slices_by_blocks_and_conflict";
	post << "block_ids=";
	
	foreach (boost::shared_ptr<Block> block, blocks)
	{
		post << delim << block->getId();
		delim = ",";
	}
	
	pt = HttpClient::postPropertyTree(url.str(), post.str());
	
	DjangoUtils::checkDjangoError(pt, url.str());
	if (pt->get_child("ok").get_value<std::string>().compare("true") != 0)
	{
		UTIL_THROW_EXCEPTION(
			DjangoException,
			"Error finding slices associated to blocks. URL: " << url.str());
	}
	slicesTree = pt->get_child("slices");
	foreach (ptree::value_type sliceV, slicesTree)
	{
		boost::shared_ptr<Slice> slice = ptreeToSlice(sliceV.second);
		slices->add(slice);
		// Make sure that the slice is in the id slice map
		if (!_idSliceMap.count(slice->getId()))
		{
			_idSliceMap[slice->getId()] = slice;
		}
	}

	return slices;
}


boost::shared_ptr<Blocks>
DjangoSliceStore::getAssociatedBlocks(boost::shared_ptr<Slice> slice)
{
	boost::shared_ptr<Blocks> blocks = boost::shared_ptr<Blocks>();

	boost::shared_ptr<ptree> pt;
	std::ostringstream url;
	std::vector<unsigned int> blockIds;
	std::string hash = getHash(*slice);
	unsigned int count;
	
	// Uncommenting this next line causes an error.
	// putSlice(slice, hash);
	
	appendProjectAndStack(url);
	url << "/blocks_by_slice?hash=" << hash;
	pt = HttpClient::getPropertyTree(url.str());
	
	// Check for problems.
	DjangoUtils::checkDjangoError(pt, url.str());
	if (pt->get_child("ok").get_value<std::string>().compare("true") != 0)
	{
		UTIL_THROW_EXCEPTION(
			DjangoException,
			"Error finding blocks associated to slices. URL: " << url.str());
	}
	
	count = HttpClient::ptreeVector<unsigned int>(pt->get_child("block_ids"), blockIds);
	
	LOG_ALL(djangoslicestorelog) << "Retrieved " << count << " blocks for slice " <<
		slice->getId() << " with hash " << getHash(*slice) << std::endl;

	blocks->addAll(_blockManager->blocksById(blockIds));
	
	return blocks;
}

void
DjangoSliceStore::storeConflict(boost::shared_ptr<ConflictSets> conflictSets)
{
	std::ostringstream url;
	std::ostringstream post;

	appendProjectAndStack(url);
	url << "/store_conflict_set";
	post << "hash=";

	std::string setdelim = "";

	foreach (const ConflictSet& conflictSet, *conflictSets)
	{
		std::ostringstream conflictSetPost;
		std::string delim = "";
		bool go = false;

		foreach (unsigned int id, conflictSet.getSlices())
		{
			if (_idSliceMap.count(id))
			{
				boost::shared_ptr<Slice> slice = _idSliceMap[id];
				
				conflictSetPost << delim << getHash(*slice);
				go = true;
				delim = ",";
			}
			else
			{
				UTIL_THROW_EXCEPTION(
					SliceCacheError,
					"Slice " << id << " not found in cache");
			}
		}

		if (go)
			// end each conflict set with a ';'
			post << setdelim << conflictSetPost.str();
			setdelim = "|";
	}

	boost::shared_ptr<ptree> pt = HttpClient::postPropertyTree(url.str(), post.str());
	DjangoUtils::checkDjangoError(pt, url.str());
	if (pt->get_child("ok").get_value<std::string>().compare("true") != 0)
	{
		UTIL_THROW_EXCEPTION(
			DjangoException,
			"Error storing conflict sets. URL: " << url.str());
	}
}

boost::shared_ptr<ConflictSets>
DjangoSliceStore::retrieveConflictSets(const Slices& slices)
{
	boost::unordered_set<ConflictSet> conflictSetSet;
	boost::shared_ptr<ConflictSets> conflictSets = boost::make_shared<ConflictSets>();

	if (slices.size() == 0)
		return conflictSets;

	std::ostringstream url;
	std::ostringstream post;
	boost::shared_ptr<ptree> pt;

	appendProjectAndStack(url);
	url << "/conflict_sets_by_slice";
	post << "hash=";

	//TODO: verify that conflict sets are returned in consistent order.

	// all but the last slice
	std::pair<Slices::const_iterator, Slices::const_iterator> firstSlices(slices.begin(), slices.end() - 1);
	foreach (boost::shared_ptr<Slice> slice, firstSlices)
	{
		std::string hash = getHash(*slice);
		post << hash << ",";
		putSlice(slice, hash);
	}
	// last slice
	std::string hash = getHash(**(slices.end() - 1));
	post << hash;
	putSlice(*(slices.end() - 1), hash);

	pt = HttpClient::postPropertyTree(url.str(), post.str());

	DjangoUtils::checkDjangoError(pt, url.str());
	
	foreach (ptree::value_type v, pt->get_child("conflict"))
	{
		boost::shared_ptr<ConflictSet> conflictSet = ptreeToConflictSet(v.second);
		conflictSetSet.insert(*conflictSet);
	}

	foreach (ConflictSet conflictSet, conflictSetSet)
	{
		conflictSets->add(conflictSet);
	}
	
	return conflictSets;
}

void
DjangoSliceStore::dumpStore()
{
	//TODO: something useful.
}

std::string
DjangoSliceStore::generateSliceHash(const Slice& slice)
{
	//TODO: create a provably near unique hash.
	return boost::lexical_cast<std::string>(slice.hashValue());
}

void
DjangoSliceStore::appendProjectAndStack(std::ostringstream& os)
{
	DjangoUtils::appendProjectAndStack(os, _server, _project, _stack);
}

void
DjangoSliceStore::putSlice(boost::shared_ptr<Slice> slice, const std::string hash)
{
	if (!_idSliceMap.count(slice->getId()))
	{
		_sliceHashMap[*slice] = hash;
		_hashSliceMap[hash] = slice;
		_idSliceMap[slice->getId()] = slice;
	}
}

std::string
DjangoSliceStore::getHash(const Slice& slice)
{
	if (_sliceHashMap.count(slice))
	{
		return _sliceHashMap[slice];
	}
	else
	{
		std::string hash = generateSliceHash(slice);
		_sliceHashMap[slice] = hash;
		return hash;
	}
}

void
DjangoSliceStore::saveConnectedComponent(std::string sliceHash, const ConnectedComponent& component)
{
	std::ofstream componentFile((_componentDirectory + "/" + sliceHash + ".cmp").c_str());

	// store the component's value
	componentFile << component.getValue() << " ";

	foreach (const util::point<unsigned int>& p, component.getPixels())
		componentFile << p.x << " " << p.y << " ";
}

boost::shared_ptr<ConnectedComponent>
DjangoSliceStore::readConnectedComponent(std::string sliceHash) {

	// create an empty pixel list
	boost::shared_ptr<ConnectedComponent::pixel_list_type> component = 
		boost::make_shared<ConnectedComponent::pixel_list_type>();

	// open the file
	std::ifstream componentFile((_componentDirectory + "/" + sliceHash + ".cmp").c_str());

	// read the component's value
	double value;
	componentFile >> value;

	// read the pixel list
	while (!componentFile.eof() && componentFile.good()) {

		unsigned int x, y;

		if (!(componentFile >> x)) break;
		if (!(componentFile >> y)) break;

		component->push_back(util::point<unsigned int>(x, y));
	}

	// Create the component
	return boost::make_shared<ConnectedComponent>(
			boost::shared_ptr<Image>(), /* no image */
			value,
			component,
			0,
			component->size());
		
}

boost::shared_ptr<Slice>
DjangoSliceStore::ptreeToSlice(const ptree& pt)
{
	std::string hash = pt.get_child("hash").get_value<std::string>();
	
	// If we have the hash in the map already, just return the already-instantiated slice.
	if (_hashSliceMap.count(hash) && !optionDjangoSliceStoreNoCache)
	{
		return _hashSliceMap[hash];
	}
	else
	{
		boost::shared_ptr<Slice> slice;
		
		unsigned int section = pt.get_child("section").get_value<unsigned int>();
		unsigned int id = ComponentTreeConverter::getNextSliceId();

		boost::shared_ptr<ConnectedComponent> component = readConnectedComponent(hash);

		// Create the slice
		slice = boost::make_shared<Slice>(id, section, component);
		
		putSlice(slice, hash);
		
		return slice;
	}
}

boost::shared_ptr<Slice>
DjangoSliceStore::sliceByHash(const std::string& hash)
{

	LOG_ALL(djangoslicestorelog) << "asking for slice with hash " << hash << std::endl;

	if (_hashSliceMap.count(hash))
	{
		LOG_ALL(djangoslicestorelog) << "slice found" << std::endl;
		return _hashSliceMap[hash];
	}
	else
	{
		LOG_ALL(djangoslicestorelog) << "this slice does not exist -- return null-pointer" << std::endl;

		boost::shared_ptr<Slice> dummySlice = boost::shared_ptr<Slice>();
		return dummySlice;
	}
}


boost::shared_ptr<ConflictSet>
DjangoSliceStore::ptreeToConflictSet(const ptree& pt)
{
	boost::shared_ptr<ConflictSet> conflictSet = boost::make_shared<ConflictSet>();
	
	ptree::value_type conflictValue = pt.front();

	if (conflictValue.first.compare("conflict_hashes") != 0)
	{
		LOG_DEBUG(djangoslicestorelog) << "Expected conflict_hashes, got " <<
			conflictValue.first << std::endl;
	}
	
	foreach (ptree::value_type vHash, conflictValue.second)
	{
		std::string hash = vHash.second.get_value<std::string>();
		unsigned int id = _hashSliceMap[hash]->getId();
		conflictSet->addSlice(id);
	}

	return conflictSet;
}

boost::shared_ptr<DjangoBlockManager>
DjangoSliceStore::getDjangoBlockManager() const
{
	return _blockManager;
}
