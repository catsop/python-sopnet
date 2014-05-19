#include "DjangoSliceStore.h"
#include "DjangoUtils.h"
#include <util/httpclient.h>
#include <sopnet/slices/ComponentTreeConverter.h>
#include <imageprocessing/ConnectedComponent.h>
#include <imageprocessing/Image.h>
#include <util/point.hpp>

logger::LogChannel djangoslicestorelog("djangoslicestorelog", "[DjangoSliceStore] ");

DjangoSliceStore::DjangoSliceStore(const boost::shared_ptr<DjangoBlockManager> blockManager) : 
	_server(blockManager->getServer()), _stack(blockManager->getStack()),
	_project(blockManager->getProject()), _blockManager(blockManager)
{
}


void
DjangoSliceStore::associate(pipeline::Value<Slices> slices, pipeline::Value<Block> block)
{
	int i = 0;
	std::ostringstream insertUrl, insertPostData, assocUrl;
	boost::shared_ptr<ptree> insertPt, assocPt;
	
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
	
	if (HttpClient::checkDjangoError(insertPt))
	{
		LOG_ERROR(djangoslicestorelog) << "Error storing slices" << std::endl;
		return;
	}
	
	// -- Step 2 : associate the slices with the given block --
	std::string delim = "";
	
	appendProjectAndStack(assocUrl);
	
	assocUrl << "/slices_block?hash=";
	
	foreach (boost::shared_ptr<Slice> slice, *slices)
	{
		assocUrl << delim << _sliceHashMap[*slice];
		delim = ",";
	}
	
	assocUrl << "&block=" << block->getId();
	
	assocPt = HttpClient::getPropertyTree(assocUrl.str());
	
	if (HttpClient::checkDjangoError(assocPt))
	{
		LOG_ERROR(djangoslicestorelog) << "Error associating slices to block " <<
			*block << std::endl;
	}
}

pipeline::Value<Slices>
DjangoSliceStore::retrieveSlices(pipeline::Value<Blocks> blocks)
{
	std::ostringstream url;
	std::string delim = "";
	boost::shared_ptr<ptree> pt;
	pipeline::Value<Slices> slices = pipeline::Value<Slices>();
	
	appendProjectAndStack(url);
	url << "/slices_by_block_and_conflict?block_ids=";
	
	foreach (boost::shared_ptr<Block> block, *blocks)
	{
		url << delim << block->getId();
		delim = ",";
	}
	
	pt = HttpClient::getPropertyTree(url.str());
	
	if (!HttpClient::checkDjangoError(pt) &&
		pt->get_child("id").get_value<std::string>().compare("true") == 0)
	{
		ptree slicesTree = pt->get_child("slices");
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
		
	}
	else
	{
		LOG_ERROR(djangoslicestorelog) << "Error retrieving slices" << std::endl;
	}
	
	return slices;
}


pipeline::Value<Blocks>
DjangoSliceStore::getAssociatedBlocks(pipeline::Value<Slice> slice)
{
	pipeline::Value<Blocks> blocks = pipeline::Value<Blocks>();

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
	if (HttpClient::checkDjangoError(pt) ||
		pt->get_child("ok").get_value<std::string>().compare("true") != 0)
	{
		LOG_ERROR(djangoslicestorelog) << "Error getting blocks for slice " << slice->getId() <<
			" with hash " << hash << std::endl;
		return blocks;
	}
	
	count = HttpClient::ptreeVector<unsigned int>(pt->get_child("block_ids"), blockIds);
	
	LOG_ALL(djangoslicestorelog) << "Retrieved " << count << " blocks for slice " <<
		slice->getId() << " with hash " << getHash(*slice) << std::endl;

	blocks->addAll(_blockManager->blocksById(blockIds));
	
	return blocks;
}

void
DjangoSliceStore::storeConflict(pipeline::Value<ConflictSets> conflictSets)
{
	foreach (const ConflictSet& conflictSet, *conflictSets)
	{
		std::ostringstream url;
		std::string delim = "";
		bool go = false;
		
		appendProjectAndStack(url);
		url << "/store_conflict_set?hash=";

		foreach (unsigned int id, conflictSet.getSlices())
		{
			if (_idSliceMap.count(id))
			{
				boost::shared_ptr<Slice> slice = _idSliceMap[id];
				
				url << delim << getHash(*slice);
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
		{
			boost::shared_ptr<ptree> pt = HttpClient::getPropertyTree(url.str());
			if (HttpClient::checkDjangoError(pt)
				|| pt->get_child("ok").get_value<std::string>().compare("true") != 0)
			{
				LOG_ERROR(djangoslicestorelog) << "Django Error while storing conflict" <<
					std::endl;
			}
		}
	}
}

pipeline::Value<ConflictSets>
DjangoSliceStore::retrieveConflictSets(pipeline::Value<Slices> slices)
{
	boost::unordered_set<ConflictSet> conflictSetSet;
	pipeline::Value<ConflictSets> conflictSets = pipeline::Value<ConflictSets>();
	
	//TODO: verify that conflict sets are returned in consistent order.
	foreach (boost::shared_ptr<Slice> slice, *slices)
	{
		std::ostringstream url;
		boost::shared_ptr<ptree> pt;
		std::string hash = getHash(*slice);
		
		putSlice(slice, hash);
		
		appendProjectAndStack(url);
		url << "/conflict_sets_by_slice?hash=" << hash;
		pt = HttpClient::getPropertyTree(url.str());
		
		if (HttpClient::checkDjangoError(pt))
		{
			LOG_ERROR(djangoslicestorelog) << "Django Error while retrieving conflict for slice "
				<< slice->getId() << " with django hash " << hash << std::endl;
			return conflictSets;
		}
		else
		{
			foreach (ptree::value_type v, pt->get_child("conflict"))
			{
				boost::shared_ptr<ConflictSet> conflictSet = ptreeToConflictSet(v.second);
				conflictSetSet.insert(*conflictSet);
			}
		}
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
	if (_hashSliceMap.count(hash))
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
		
		std::vector<unsigned int> vX, vY;
		// We expect vX.size() == vY.size(), but take the min for safety.
		unsigned int n = vX.size() <= vY.size() ? vX.size() : vY.size();
		
		// Parse variables from the ptree
		HttpClient::ptreeVector<unsigned int>(pt.get_child("x"), vX);
		HttpClient::ptreeVector<unsigned int>(pt.get_child("y"), vY);
		
		// Fill the pixel list
		for (unsigned int i = 0; i < n; ++i)
		{
			pixelList->push_back(util::point<unsigned int>(vX[i], vY[i]));
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

boost::shared_ptr<ConflictSet>
DjangoSliceStore::ptreeToConflictSet(const ptree& pt)
{
	boost::shared_ptr<ConflictSet> conflictSet = boost::make_shared<ConflictSet>();
	
	foreach (ptree::value_type vConflict, pt)
	{
		ptree hashes = vConflict.second.front().second;
		foreach (ptree::value_type vHash, hashes)
		{
			std::string hash = vHash.second.get_value<std::string>();
			unsigned int id = _hashSliceMap[hash]->getId();
			conflictSet->addSlice(id);
		}
	}
	
	return conflictSet;
}

