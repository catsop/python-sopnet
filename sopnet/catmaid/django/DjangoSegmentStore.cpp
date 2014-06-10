#include "DjangoSegmentStore.h"
#include <util/httpclient.h>
#include <util/Logger.h>
#include <catmaid/django/DjangoUtils.h>

logger::LogChannel djangosegmentstorelog("djangosegmentstorelog", "[DjangoSegmentStore] ");

DjangoSegmentStore::DjangoSegmentStore(boost::shared_ptr<DjangoSliceStore> sliceStore):
	_sliceStore(sliceStore),
	_server(_sliceStore->getDjangoBlockManager()->getServer()),
	_project(_sliceStore->getDjangoBlockManager()->getProject()),
	_stack(_sliceStore->getDjangoBlockManager()->getStack()),
	_featureNamesFlag(false)
{

}

void
DjangoSegmentStore::associate(pipeline::Value<Segments> segments,
							  pipeline::Value<Block> block)
{
	int i = 0;
	std::ostringstream insertUrl, insertPostData, assocUrl;
	boost::shared_ptr<ptree> insertPt, assocPt;
	
	appendProjectAndStack(insertUrl);
	insertUrl << "/insert_segments";
	
	insertPostData<< "n=" << segments->size();
	foreach (boost::shared_ptr<Segment> segment, segments->getSegments())
	{
		util::point<double> ctr = segment->getCenter();
		util::rect<int> segmentBound = DjangoUtils::segmentBound(segment);
		std::string hash = getHash(segment);
		int type = getSegmentType(segment);
		int direction = getSegmentDirection(segment);
		unsigned int section = getSectionInfimum(segment);
		
		putSegment(segment, hash);
		
		insertPostData << "&hash_" << i << "=" << hash;
		insertPostData << "&sectioninf_" << i << "=" << section;
		insertPostData << "&cx_" << i << "=" << ctr.x;
		insertPostData << "&cy_" << i << "=" << ctr.y;
		insertPostData << "&minx_" << i << "=" << segmentBound.minX;
		insertPostData << "&maxx_" << i << "=" << segmentBound.maxX;
		insertPostData << "&miny_" << i << "=" << segmentBound.minY;
		insertPostData << "&maxy_" << i << "=" << segmentBound.maxY;
		insertPostData << "&type_" << i << "=" << type;
		insertPostData << "&direction_" << i << "=" << direction;
		
		switch(type)
		{
			case 2:
				insertPostData << "&slice_c_" << i << "=" <<
					_sliceStore->getHash(segment->getSlices()[2]);
			case 1:
				insertPostData << "&slice_b_" << i << "=" <<
					_sliceStore->getHash(segment->getSlices()[1]);
			case 0:
				insertPostData << "&slice_a_" << i << "=" <<
					_sliceStore->getHash(segment->getSlices()[0]);
				break;
		}

		++i;
	}
	
	insertPt = HttpClient::postPropertyTree(insertUrl.str(), insertPostData.str());
	
	if (HttpClient::checkDjangoError(insertPt))
	{
		LOG_ERROR(djangosegmentstorelog) << "Error storing segments" << std::endl;
		LOG_ERROR(djangosegmentstorelog) << "\tURL was" << insertUrl.str() << std::endl;
		LOG_ERROR(djangosegmentstorelog) << "\tData was\n\t" << insertPostData.str() << std::endl;
		return;
	}
	
	std::string delim = "";
	
	appendProjectAndStack(assocUrl);
	
	assocUrl << "/segments_block?hash=";
	
	foreach (boost::shared_ptr<Segment> segment, segments->getSegments())
	{
		assocUrl << delim << getHash(segment);
		delim = ",";
	}
	
	assocUrl << "&block=" << block->getId();
	
	assocPt = HttpClient::getPropertyTree(assocUrl.str());
	
	if (HttpClient::checkDjangoError(assocPt))
	{
		LOG_ERROR(djangosegmentstorelog) << "Error associated segments to block " <<
			*block << std::endl;
	}
}

pipeline::Value<Segments>
DjangoSegmentStore::retrieveSegments(pipeline::Value<Blocks> blocks)
{
	std::ostringstream url;
	std::string delim = "";
	boost::shared_ptr<ptree> pt;
	pipeline::Value<Segments> segments = pipeline::Value<Segments>();
	
	appendProjectAndStack(url);
	url << "segments_by_blocks?block_ids=";
	
	foreach (boost::shared_ptr<Block> block, *blocks)
	{
		url << delim << block->getId();
		delim = ",";
	}
	
	pt = HttpClient::getPropertyTree(url.str());
	
	if (!HttpClient::checkDjangoError(pt) &&
		pt->get_child("ok").get_value<std::string>().compare("true") == 0)
	{
		ptree segmentsTree = pt->get_child("segments");
		foreach (ptree::value_type segmentV, segmentsTree)
		{
			boost::shared_ptr<Segment> segment = ptreeToSegment(segmentV.second);
			segments->add(segment);
			if (!_idSegmentMap.count(segment->getId()))
			{
				_idSegmentMap[segment->getId()] = segment;
			}
		}
	}
	
	return segments;
}

pipeline::Value<Blocks>
DjangoSegmentStore::getAssociatedBlocks(pipeline::Value<Segment> segment)
{
	pipeline::Value<Blocks> blocks = pipeline::Value<Blocks>();

	boost::shared_ptr<ptree> pt;
	std::ostringstream url;
	std::vector<unsigned int> blockIds;
	std::string hash = getHash(*segment);
	unsigned int count;
	
	putSegment(segment, hash);
	
	appendProjectAndStack(url);
	url << "/blocks_by_segment?hash=" << hash;
	pt = HttpClient::getPropertyTree(url.str());
	
	// Check for problems.
	if (HttpClient::checkDjangoError(pt) ||
		pt->get_child("ok").get_value<std::string>().compare("true") != 0)
	{
		LOG_ERROR(djangoslicestorelog) << "Error getting blocks for segment " << segment->getId() <<
			" with hash " << hash << std::endl;
		return blocks;
	}
	
	count = HttpClient::ptreeVector<unsigned int>(pt->get_child("block_ids"), blockIds);
	
	LOG_ALL(djangoslicestorelog) << "Retrieved " << count << " blocks for segment " <<
		segment->getId() << " with hash " << getHash(segment) << std::endl;

	blocks->addAll(_sliceStore->getDjangoBlockManager()->blocksById(blockIds));
	
	return blocks;
}

int DjangoSegmentStore::storeFeatures(pipeline::Value<Features> features)
{
	unsigned int i = 0;
	int count;
	std::map<unsigned int, unsigned int> idMap = features->getSegmentsIdsMap();
	std::map<unsigned int, unsigned int>::const_iterator it;
	std::ostringstream url;
	std::ostringstream data;
	boost::shared_ptr<ptree> pt;
	
	setFeatureNames(features->getNames());
	
	appendProjectAndStack(url);
	
	for (it = idMap.begin(); it != idMap.end(); ++it)
	{
		unsigned int j = it->second;
		unsigned int id = it->first;
		if (_idSegmentMap.count(id))
		{
			boost::shared_ptr<Segment> segment = _idSegmentMap[id];
			std::vector<double> fv = (*features)[j];
			std::string delim = "";
			
			data << "hash_" << i << "=" << getHash(segment) << "&features_" << i << "=";
			foreach (double v, fv)
			{
				data << delim << v;
				delim = ",";
			}
			data << "&";
			++i;
		}
	}
	
	data << "n=" << i;
	
	pt = HttpClient::postPropertyTree(url.str(), data.str());
	
	if (HttpClient::checkDjangoError(pt))
	{
		LOG_ERROR(djangosegmentstorelog) << "Error storing features" << std::endl;
		LOG_ERROR(djangosegmentstorelog) << "\tURL was" << url.str() << std::endl;
		LOG_ERROR(djangosegmentstorelog) << "\tData was\n\t" << data.str() << std::endl;
		
		if (HttpClient::ptreeHasChild(pt, "count"))
		{
			return pt->get_child("count").get_value<int>();
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return pt->get_child("count").get_value<int>();
	}
}

pipeline::Value<SegmentStore::SegmentFeaturesMap>
DjangoSegmentStore::retrieveFeatures(pipeline::Value<Segments> segments)
{
	pipeline::Value<SegmentStore::SegmentFeaturesMap> featureMap = 
		pipeline::Value<SegmentStore::SegmentFeaturesMap>();
	boost::shared_ptr<ptree> pt;
	std::ostringstream url;
	std::string delim = "";
	
	appendProjectAndStack(url);
	
	url << "/features_by_segments?hash=";
	
	foreach (boost::shared_ptr<Segment> segment, segments->getSegments())
	{
		std::string hash = getHash(segment);
		url << delim << hash;
		delim = ",";
		
		// Put the segments into the map for later.
		// This step is probably unnecessary because any Segments for which we want the features
		// should already be in the map, but I think it will help us to avoid some finnicky
		// problems that may arise.
		
		putSegment(segment, hash);
	}
	
	pt = HttpClient::getPropertyTree(url.str());
	
	if (HttpClient::checkDjangoError(pt))
	{
		LOG_ERROR(djangosegmentstorelog) << "Error while retrieving features" << std::endl;
		LOG_ERROR(djangosegmentstorelog) << "\tURL was " << url.str() << std::endl;
		return featureMap;
	}
	else
	{
		ptree featuresTree = pt->get_child("features");
		foreach (ptree::value_type featureNode, featuresTree)
		{
			
			std::string hash = featureNode.second.get_child("hash").get_value<std::string>();

			if (_hashSegmentMap.count(hash))
			{
				std::vector<double> fvect;
				HttpClient::ptreeVector<double>(featureNode.second.get_child("fv"), fvect);
				(*featureMap)[_hashSegmentMap[hash]] = fvect;
			}
		}
	}
	
	return featureMap;
}

std::vector<std::string>
DjangoSegmentStore::getFeatureNames()
{
	std::ostringstream url;
	boost::shared_ptr<ptree> pt;
	std::vector<std::string> featureNames;
	
	appendProjectAndStack(url);
	
	url << "/feature_names";
	
	pt = HttpClient::getPropertyTree(url.str());
	
	if (HttpClient::checkDjangoError(pt))
	{
		LOG_ERROR(djangosegmentstorelog) << "Error reading feature names from url " << url.str()
			<< std::endl;
		return featureNames;
	}
	
	foreach (ptree::value_type nameNode, pt->get_child("names"))
	{
		featureNames.push_back(nameNode.second.get_value<std::string>());
	}
	
	return featureNames;
}

unsigned int
DjangoSegmentStore::storeCost(pipeline::Value<Segments> segments,
							  pipeline::Value<LinearObjective> objective)
{
	
}


int
DjangoSegmentStore::getSegmentDirection(const boost::shared_ptr<Segment> segment)
{
	switch (segment->getDirection())
		{
			case Left:
				return 0;
			case Right:
				return 1;
			default:
				return -1;
		}
}

int
DjangoSegmentStore::getSegmentType(const boost::shared_ptr<Segment> segment)
{
	switch (segment->getType())
	{
		case EndSegmentType:
			return 0;
		case ContinuationSegmentType:
			return 1;
		case BranchSegmentType:
			return 2;
		default:
			return -1;
	}
}

void DjangoSegmentStore::appendProjectAndStack(std::ostringstream& os)
{
	DjangoUtils::appendProjectAndStack(os, _server, _project, _stack);
}
