#include "DjangoSegmentStore.h"
#include <util/httpclient.h>
#include <util/Logger.h>
#include <catmaid/django/DjangoUtils.h>
#include <boost/algorithm/string/replace.hpp>

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
	if (segments->size() == 0)
		return;

	{
		int i = 0;
		std::ostringstream insertUrl, insertPostData;
		boost::shared_ptr<ptree> insertPt;
		
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
						_sliceStore->getHash(*segment->getSlices()[2]);
				case 1:
					insertPostData << "&slice_b_" << i << "=" <<
						_sliceStore->getHash(*segment->getSlices()[1]);
				case 0:
					insertPostData << "&slice_a_" << i << "=" <<
						_sliceStore->getHash(*segment->getSlices()[0]);
					break;
			}

			++i;
		}
		
		insertPt = HttpClient::postPropertyTree(insertUrl.str(), insertPostData.str());
		
		if (HttpClient::checkDjangoError(insertPt))
		{
			LOG_ERROR(djangosegmentstorelog) << "Error storing segments" << std::endl;
			LOG_ERROR(djangosegmentstorelog) << "\tURL was " << insertUrl.str() << std::endl;
			LOG_ERROR(djangosegmentstorelog) << "\tData was\n\t" << insertPostData.str() << std::endl;
			return;
		}
	}
	
	{
		std::ostringstream assocUrl, assocPost;
		std::string delim = "";
		boost::shared_ptr<ptree> assocPt;
		
		appendProjectAndStack(assocUrl);
		
		assocUrl << "/segments_block";
		assocPost << "hash=";
		
		foreach (boost::shared_ptr<Segment> segment, segments->getSegments())
		{
			assocPost << delim << getHash(segment);
			delim = ",";
		}
		
		assocPost << "&block=" << block->getId();
		
		assocPt = HttpClient::postPropertyTree(assocUrl.str(), assocPost.str());
		
		if (HttpClient::checkDjangoError(assocPt))
		{
			LOG_ERROR(djangosegmentstorelog)
					<< "Error associated segments to block "
					<< *block << std::endl;
			return;
		}
	}
}

pipeline::Value<Segments>
DjangoSegmentStore::retrieveSegments(const Blocks& blocks)
{
	std::ostringstream url;
	std::ostringstream post;
	std::string delim = "";
	boost::shared_ptr<ptree> pt;
	pipeline::Value<Segments> segments = pipeline::Value<Segments>();
	
	appendProjectAndStack(url);
	url << "/segments_by_blocks";
	post << "block_ids=";
	
	foreach (boost::shared_ptr<Block> block, blocks)
	{
		post << delim << block->getId();
		delim = ",";
	}

	LOG_DEBUG(djangosegmentstorelog)
			<< "requesting segments from " << url.str()
			<< ", " << post.str() << std::endl;
	
	pt = HttpClient::postPropertyTree(url.str(), post.str());
	
	if (!HttpClient::checkDjangoError(pt) &&
		pt->get_child("ok").get_value<std::string>().compare("true") == 0)
	{
		ptree segmentsTree = pt->get_child("segments");

		LOG_DEBUG(djangosegmentstorelog)
			<< "requesting all slices in the same blocks" << std::endl;
		
		// Force the slice store to cache the necessary slices.
		_sliceStore->retrieveSlices(blocks);

		LOG_DEBUG(djangosegmentstorelog) << "create segments from ptree" << std::endl;
		
		foreach (ptree::value_type segmentV, segmentsTree)
		{
			boost::shared_ptr<Segment> segment = ptreeToSegment(segmentV.second);
			segments->add(segment);
			if (!_idSegmentMap.count(segment->getId()))
			{
				_idSegmentMap[segment->getId()] = segment;
			}
		}

		LOG_DEBUG(djangosegmentstorelog) << "done" << std::endl;
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
	std::string hash = getHash(segment);
	unsigned int count;
	
	appendProjectAndStack(url);
	url << "/blocks_by_segment?hash=" << hash;
	pt = HttpClient::getPropertyTree(url.str());
	
	// Check for problems.
	if (HttpClient::checkDjangoError(pt) ||
		pt->get_child("ok").get_value<std::string>().compare("true") != 0)
	{
		LOG_ERROR(djangosegmentstorelog) << "Error getting blocks for segment " << segment->getId() <<
			" with hash " << hash << std::endl;
		return blocks;
	}
	
	count = HttpClient::ptreeVector<unsigned int>(pt->get_child("block_ids"), blockIds);
	
	LOG_ALL(djangosegmentstorelog) << "Retrieved " << count << " blocks for segment " <<
		segment->getId() << " with hash " << getHash(segment) << std::endl;

	blocks->addAll(_sliceStore->getDjangoBlockManager()->blocksById(blockIds));
	
	return blocks;
}

int DjangoSegmentStore::storeFeatures(pipeline::Value<Features> features)
{
	unsigned int i = 0;
	std::map<unsigned int, unsigned int> idMap = features->getSegmentsIdsMap();
	std::map<unsigned int, unsigned int>::const_iterator it;
	std::ostringstream url;
	std::ostringstream data;
	boost::shared_ptr<ptree> pt;
	
	setFeatureNames(features->getNames());
	
	appendProjectAndStack(url);
	url << "/store_segment_features";
	
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
		LOG_ERROR(djangosegmentstorelog) << "\tURL was " << url.str() << std::endl;
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
	std::ostringstream post;
	std::string delim = "";
	
	appendProjectAndStack(url);
	
	url << "/features_by_segments";
	post << "hash=";
	
	foreach (boost::shared_ptr<Segment> segment, segments->getSegments())
	{
		std::string hash = getHash(segment);
		post << delim << hash;
		delim = ",";
	}
	
	pt = HttpClient::postPropertyTree(url.str(), post.str());
	
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
	unsigned int i = 0;
	const std::vector<double> coefs = objective->getCoefficients();
	std::ostringstream url;
	std::ostringstream post;
	boost::shared_ptr<ptree> pt;
	
	appendProjectAndStack(url);
	url << "/store_segment_costs";
	post << "n=" << segments->size();
	
	foreach (boost::shared_ptr<Segment> segment, segments->getSegments())
	{
		post << "&hash_" << i << "=" << getHash(segment);
		post << "&cost_" << i << "=" << coefs[i];
		
		++i;
		
		if (i > coefs.size())
		{
			break;
		}
	}
	
	pt = HttpClient::postPropertyTree(url.str(), post.str());
	
	if (HttpClient::checkDjangoError(pt))
	{
		LOG_ERROR(djangosegmentstorelog) << "Error while storing segment costs" << std::endl;
		LOG_ERROR(djangosegmentstorelog) << "\tURL was:\n\t" << url.str() << std::endl;
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

pipeline::Value<LinearObjective>
DjangoSegmentStore::retrieveCost(pipeline::Value<Segments> segments,
								 double defaultCost, pipeline::Value<Segments> segmentsNF)
{
	pipeline::Value<LinearObjective> objective = pipeline::Value<LinearObjective>();
	std::ostringstream url;
	std::ostringstream post;
	boost::shared_ptr<ptree> pt;
	std::string delim = "";
	std::map<std::string, double> hashCostMap;
	
	appendProjectAndStack(url);
	url << "/costs_by_segments";
	post << "hash=";
	
	
	foreach (boost::shared_ptr<Segment> segment, segments->getSegments())
	{
		std::string hash = getHash(segment);
		
		post << delim << hash;
		delim = ",";
	}
	
	pt = HttpClient::postPropertyTree(url.str(), post.str());
	
	if (!HttpClient::checkDjangoError(pt) &&
		pt->get_child("ok").get_value<std::string>().compare("true") == 0)
	{
		unsigned int i = 0;
		ptree costTree = pt->get_child("costs");
		
		objective->resize(segments->size());
		
		// Populate the hash->cost map from the result
		foreach (ptree::value_type costNode, costTree)
		{
			std::string hash = costNode.second.get_child("hash").get_value<std::string>();
			double cost = costNode.second.get_child("cost").get_value<double>();
			
			hashCostMap[hash] = cost;
		}
		
		// Iterate through the segments, pushing their cost to the objective, if it exists
		foreach (boost::shared_ptr<Segment> segment, segments->getSegments())
		{
			std::string hash = getHash(segment);
			if (hashCostMap.count(hash))
			{
				objective->setCoefficient(i, hashCostMap[hash]);
			}
			else
			{
				objective->setCoefficient(i, defaultCost);
				segmentsNF->add(segment);
			}
			++i;
		}
		
	}
	else
	{
		LOG_ERROR(djangosegmentstorelog) << "Error while retrieving segment costs from url " <<
			url.str() << std::endl;
	}
	
	return objective;
}

unsigned int
DjangoSegmentStore::storeSolution(pipeline::Value<Segments> segments,
								  pipeline::Value<Core> core,
								  pipeline::Value<Solution> solution,
								  std::vector<unsigned int> indices)
{
	unsigned int count = 0, i = 0;
	std::ostringstream url;
	std::ostringstream post;
	boost::shared_ptr<ptree> pt;
	
	appendProjectAndStack(url);
	url << "/store_segment_solutions";
	post << "n=" << segments->size() << "&core_id=" << core->getId();

	if (solution->size() == 0 || indices.empty())
	{
		return 0;
	}
	
	foreach (boost::shared_ptr<Segment> segment, segments->getSegments())
	{
		
		std::string hash = getHash(segment);
		unsigned int j = indices[i];
		bool bSolution = (*solution)[j] == 1.0;
		
		post << "&hash_" << i << "=" << hash;
		post << "&solution_" << i << "=" << bSolution;
		++i;
	}
	
	pt = HttpClient::postPropertyTree(url.str(), post.str());
	
	if (HttpClient::checkDjangoError(pt))
	{
		LOG_ERROR(djangosegmentstorelog) << "Error while storing segment solutions" << std::endl;
		LOG_ERROR(djangosegmentstorelog) << "\tURL was:\n\t" << url.str() << std::endl;
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
	}

	return count;
}

pipeline::Value<Solution>
DjangoSegmentStore::retrieveSolution(pipeline::Value<Segments> segments,
									 pipeline::Value<Core> core)
{
	std::ostringstream url;
	std::ostringstream post;
	boost::shared_ptr<ptree> pt;
	std::string delim = "";
	pipeline::Value<Solution> solution = pipeline::Value<Solution>();
	
	appendProjectAndStack(url);
	url << "/solutions_by_core_and_segments";
	post << "core_id=" << core->getId() << "&hash=";
	
	foreach (boost::shared_ptr<Segment> segment, segments->getSegments())
	{
		post << delim << getHash(segment);
		delim = ",";
	}
	
	pt = HttpClient::postPropertyTree(url.str(), post.str());
	
	if (!HttpClient::checkDjangoError(pt))
	{
		std::map<std::string, double> solutionMap;
		ptree solutionsTree = pt->get_child("solutions");
		unsigned int i = 0;
		
		solution->resize(segments->size());
		
		foreach (ptree::value_type solutionNode, solutionsTree)
		{
			std::string hash = solutionNode.second.get_child("hash").get_value<std::string>();
			double solution = solutionNode.second.get_child(
				"solution").get_value<std::string>().compare("true") == 0 ? 1.0 : 0.0;
			solutionMap[hash] = solution;
		}
		
		foreach (boost::shared_ptr<Segment> segment, segments->getSegments())
		{
			std::string hash = getHash(segment);
			if (solutionMap.count(hash))
			{
				(*solution)[i] = solutionMap[hash];
			}
			else
			{
				(*solution)[i] = 0;
			}
			++i;
		}
	}
	else
	{
		LOG_ERROR(djangosegmentstorelog) << "Error while retrieving solutions" << std::endl;
		LOG_ERROR(djangosegmentstorelog) << "\tURL was " << url.str() << std::endl;
	}
	
	return solution;
}


void DjangoSegmentStore::dumpStore()
{

}

unsigned int
DjangoSegmentStore::getSectionInfimum(const boost::shared_ptr<Segment> segment)
{
	unsigned int sinf = segment->getSlices()[0]->getSection();
	foreach (boost::shared_ptr<Slice> slice, segment->getSlices())
	{
		unsigned int s = slice->getSection();
		if (sinf > s)
		{
			sinf = s;
		}
	}
	
	return sinf;
}

std::string
DjangoSegmentStore::getHash(const boost::shared_ptr<Segment> segment)
{
	if (_segmentHashMap.count(segment))
	{
		return _segmentHashMap[segment];
	}
	else
	{
		std::string hash = generateHash(segment);
		_segmentHashMap[segment] = hash;
		_hashSegmentMap[hash] = segment;
		return hash;
	}
}

std::string
DjangoSegmentStore::generateHash(const boost::shared_ptr<Segment> segment)
{
	//TODO: generate a provably unique hash
	std::stringstream ss;
	ss << segment->hashValue();
	return ss.str();
}

void
DjangoSegmentStore::setFeatureNames(const std::vector<std::string>& featureNames)
{
	if (!_featureNamesFlag)
	{
		std::ostringstream url;
		std::ostringstream post;
		boost::shared_ptr<ptree> pt;
		std::string delim = "";
		
		appendProjectAndStack(url);
		
		url << "/set_feature_names";
		post << "names=";
		
		foreach (std::string name, featureNames)
		{
			boost::replace_all(name, " ", "+");
			boost::replace_all(name, ",", "+");
			boost::replace_all(name, "&", "+");
			post << delim << name;
			delim = ",";
		}
		
		LOG_DEBUG(djangosegmentstorelog) << "Setting feature names via url: " << url.str() << ", post: " << post.str() << std::endl;
		
		pt = HttpClient::postPropertyTree(url.str(), post.str());
		
		if (HttpClient::checkDjangoError(pt) ||
			pt->get_child("ok").get_value<std::string>().compare("true") != 0)
		{
			LOG_ERROR(djangosegmentstorelog) << "Error while setting feature names" << std::endl;
			LOG_ERROR(djangosegmentstorelog) << "\tURL was " << url.str() << std::endl;
		}
		else
		{
			_featureNamesFlag = true;
		}
	}
}

boost::shared_ptr<Segment> DjangoSegmentStore::ptreeToSegment(const ptree& pt)
{
	std::string hash = pt.get_child("hash").get_value<std::string>();
	if (_hashSegmentMap.count(hash))
	{
		return _hashSegmentMap[hash];
	}
	else
	{
		boost::shared_ptr<Slice> sliceA, sliceB, sliceC;
		boost::shared_ptr<Segment> segment;
		int iDir = pt.get_child("direction").get_value<int>();
		int type = pt.get_child("type").get_value<int>();
		Direction direction = iDir == 0 ? Left : Right;
		unsigned int id = Segment::getNextSegmentId();

		LOG_ALL(djangosegmentstorelog) << "creating a segment of type " << type << std::endl;
		
		switch(type)
		{
			case 2:
				sliceC = _sliceStore->sliceByHash(
					pt.get_child("slice_c").get_value<std::string>());
			case 1:
				sliceB = _sliceStore->sliceByHash(
					pt.get_child("slice_b").get_value<std::string>());
			case 0:
				sliceA = _sliceStore->sliceByHash(
					pt.get_child("slice_a").get_value<std::string>());
		}
		
		switch(type)
		{
			case 0:
				segment = boost::make_shared<EndSegment>(id, direction, sliceA);
				break;
			case 1:
				segment = boost::make_shared<ContinuationSegment>(id, direction, sliceA, sliceB);
				break;
			case 2:
				segment = boost::make_shared<BranchSegment>(id, direction, sliceA, sliceB, sliceC);
				break;
			default:
				LOG_ERROR(djangosegmentstorelog) << "Got unknown segment type: " <<
					type << " for segment with hash " << hash << std::endl;
				segment = boost::shared_ptr<Segment>();
				break;
		}
		
		_hashSegmentMap[hash] = segment;
		_segmentHashMap[segment] = hash;

		LOG_ALL(djangosegmentstorelog) << "done" << std::endl;;
		
		return segment;
	}
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
