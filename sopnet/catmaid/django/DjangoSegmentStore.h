#ifndef DJANGO_SEGMENT_STORE_H__
#define DJANGO_SEGMENT_STORE_H__

#include <boost/unordered_map.hpp>
#include <catmaid/persistence/SegmentPointerHash.h>
#include <catmaid/persistence/SegmentStore.h>
#include "DjangoSliceStore.h"

class DjangoSegmentStore : public SegmentStore
{
public:
	DjangoSegmentStore(boost::shared_ptr<DjangoSliceStore> sliceStore);
	
	void associate(pipeline::Value<Segments> segments, pipeline::Value<Block> block);

	pipeline::Value<Segments> retrieveSegments(pipeline::Value<Blocks> blocks);

	pipeline::Value<Blocks> getAssociatedBlocks(pipeline::Value<Segment> segment);

	int storeFeatures(pipeline::Value<Features> features);

	pipeline::Value<SegmentFeaturesMap> retrieveFeatures(pipeline::Value<Segments> segments);

	std::vector<std::string> getFeatureNames();

	unsigned int storeCost(pipeline::Value<Segments> segments,
							   pipeline::Value<LinearObjective> objective);

	pipeline::Value<LinearObjective> retrieveCost(pipeline::Value<Segments> segments,
														  double defaultCost,
														  pipeline::Value<Segments> segmentsNF);

	unsigned int storeSolution(pipeline::Value<Segments> segments,
									   pipeline::Value<Core> core,
									   pipeline::Value<Solution> solution,
									   std::vector<unsigned int> indices);

	pipeline::Value<Solution> retrieveSolution(pipeline::Value<Segments> segments,
													   pipeline::Value<Core> core);

	void dumpStore();

private:
	void appendProjectAndStack(std::ostringstream& os);
	
	void putSegment(const boost::shared_ptr<Segment> segment,
					const std::string hash);
	
	std::string getHash(const boost::shared_ptr<Segment> segment);
	
	void setFeatureNames(const std::vector<std::string>& featureNames);
	
	static int getSegmentType(const boost::shared_ptr<Segment> segment);
	static int getSegmentDirection(const boost::shared_ptr<Segment> segment);
	static unsigned int getSectionInfimum(const boost::shared_ptr<Segment> segment);
	
	static boost::shared_ptr<Segment> ptreeToSegment(const boost::property_tree::ptree& pt);
	
	const boost::shared_ptr<DjangoSliceStore> _sliceStore;
	const std::string _server;
	const unsigned int _project, _stack;
	const boost::unordered_map<boost::shared_ptr<Segment>, std::string,
		SegmentPointerHash, SegmentPointerEquals> _segmentHashMap;
	const std::map<std::string, boost::shared_ptr<Segment> > _hashSegmentMap;
	const std::map<unsigned int, boost::shared_ptr<Segment> > _idSegmentMap;
	bool _featureNamesFlag;
};


#endif //DJANGO_SEGMENT_STORE_H__