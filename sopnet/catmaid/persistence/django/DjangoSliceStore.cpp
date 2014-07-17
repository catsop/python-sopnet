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


DjangoSliceStore::DjangoSliceStore(const boost::shared_ptr<DjangoBlockManager> blockManager) : 
	_server(blockManager->getServer()), _stack(blockManager->getStack()),
	_project(blockManager->getProject()), _blockManager(blockManager)
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
			
			appendGeometry(slice->getComponent(), osX, osY);

			// Section
			insertPostData << "&section_" << i << "=" << slice->getSection();
			// Hash
			insertPostData << "&hash_" << i << "=" << hash;
			// Centroid
			insertPostData << "&cx_" << i << "=" << ctr.x;
			insertPostData << "&cy_" << i << "=" << ctr.y;
			// Geometry
			insertPostData << "&x_" << i << "=" << osX.str();
			insertPostData << "&y_" << i << "=" << osY.str();
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
	boost::shared_ptr<Slices> slices = boost::shared_ptr<Slices>();
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
				LOG_DEBUG(djangoslicestorelog) << "Slice " << id << " not found in cache" <<
					std::endl;
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
	boost::shared_ptr<ConflictSets> conflictSets = boost::shared_ptr<ConflictSets>();

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
DjangoSliceStore::appendGeometry(const boost::shared_ptr<ConnectedComponent> component,
								 std::ostringstream& osX, std::ostringstream& osY)
{
	//TODO: RLE
	
	ConnectedComponent::bitmap_type bitmap = component->getBitmap();
	util::rect<int> box = component->getBoundingBox();
	int minX = box.minX;
	int minY = box.minY;
	std::string delim = "";
	
	for (int y = minY; y < box.maxY; ++y)
	{
		for (int x = minX; x < box.maxX; ++x)
		{
			if (bitmap(x - minX, y - minY))
			{
				osX << delim << x;
				osY << delim << y;
				delim = ",";
			}
		}
	}
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
		boost::shared_ptr<ConnectedComponent> component;
		boost::shared_ptr<Image> nullImage = boost::shared_ptr<Image>();
		boost::shared_ptr<ConnectedComponent::pixel_list_type> pixelList = 
			boost::make_shared<ConnectedComponent::pixel_list_type>();
		
		unsigned int section = pt.get_child("section").get_value<unsigned int>();
		double value = pt.get_child("value").get_value<double>();
		unsigned int id = ComponentTreeConverter::getNextSliceId();
		
		std::vector<unsigned int> pixelListX, pixelListY;
		
		// Parse variables from the ptree
		HttpClient::ptreeVector<unsigned int>(pt.get_child("x"), pixelListX);
		HttpClient::ptreeVector<unsigned int>(pt.get_child("y"), pixelListY);

		if (pixelListX.size() != pixelListY.size())
			UTIL_THROW_EXCEPTION(
					IOError,
					"pixel lists for x and y in django answer are not of same size: " <<
					pixelListX.size() << " (x) vs. " << pixelListY.size() << "(y)");

		unsigned int n = pixelListX.size();
		
		// Fill the pixel list
		for (unsigned int i = 0; i < n; ++i)
		{
			pixelList->push_back(util::point<unsigned int>(pixelListX[i], pixelListY[i]));
		}
		
		// Create the component
		component = boost::make_shared<ConnectedComponent>(nullImage, value, pixelList, 0,
														   pixelList->size());
		
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
