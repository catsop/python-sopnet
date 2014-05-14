#include "DjangoSliceStore.h"
#include "DjangoUtils.h"
#include <util/httpclient.h>

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
		// TODO: check for slices that are already in the db (don't send these).
		
		std::ostringstream osX, osY;
		std::string hash = getHash(slice);
		util::point<double> ctr = slice->getComponent()->getCenter();
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
	
	foreach (boost::shared_ptr<Slice> slice, slices)
	{
		assocUrl << delim << _sliceHashMap[slice];
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
		foreach (ptree sliceTree, slicesTree)
		{
			slices->add(ptreeToSlice(slicetree));
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
	unsigned int count;
	
	appendProjectAndStack(url);
	url << "/blocks_by_slice?hash=" << getHash(slice);
	pt = HttpClient::getPropertyTree(url.str());
	
	// Check for problems.
	if (HttpClient::checkDjangoError(pt) ||
		pt->get_child("ok").get_value<std::string().compare("true") != 0)
	{
		LOG_ERROR(djangoslicestorelog) << "Error getting blocks for slice " << slice->getId() <<
			" with hash " << getHash(slice) << std::endl;
		return blocks;
	}
	
	count = HttpClient::ptreeVector<unsigned int>(pt->get_child("block_ids"));
	
	LOG_ALL(djangoslicestorelog) << "Retrieved " << count << " blocks for slice " <<
		slice->getId() << " with hash " << getHash(slice) << std::endl;
	
	foreach (unsigned int id, blockIds)
	{
		blocks->add(_blockManager->blockById(id));
	}
	
	return blocks;
}

void
DjangoSliceStore::storeConflict(pipeline::Value<ConflictSets> conflictSets)
{

}

pipeline::Value<ConflictSets>
DjangoSliceStore::retrieveConflictSets(pipeline::Value<Slices> slices)
{

}

void
DjangoSliceStore::dumpStore()
{

}

std::string
DjangoSliceStore::generateSliceHash(const boost::shared_ptr<Slice> slice)
{

}

void
DjangoSliceStore::appendProjectAndStack(std::ostringstream& os)
{
	DjangoUtils::appendProjectAndStack(os, _server, _project, _stack);
}

void
DjangoSliceStore::putSlice(const boost::shared_ptr<Slice> slice, const std::string hash)
{
	_sliceHashMap[slice] = hash;
	_hashSliceMap[hash] = slice;
}

std::string
DjangoSliceStore::getHash(const boost::shared_ptr< Slice > slice)
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

}

boost::shared_ptr<Slice>
DjangoSliceStore::ptreeToSlice(const ptree& pt)
{

}
