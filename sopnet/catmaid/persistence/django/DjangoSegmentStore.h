#ifndef DJANGO_SEGMENT_STORE_H__
#define DJANGO_SEGMENT_STORE_H__

#include <boost/unordered_map.hpp>
#include <catmaid/persistence/SegmentPointerHash.h>
#include <catmaid/persistence/SegmentStore.h>
#include "DjangoSliceStore.h"

/**
 * DjangoSegmentStore is a CATMAID/Django-backed segment store. Segments, Features,
 * LinearObjectives, and Solutions are stored in the django database and transferred via http
 * requests.
 */
class DjangoSegmentStore : public SegmentStore
{
public:
	/**
	 * Construct a DjangoSegmentStore from a DjangoSliceStore.
	 */
	DjangoSegmentStore(boost::shared_ptr<DjangoSliceStore> sliceStore);
	
	void associate(pipeline::Value<Segments> segments, pipeline::Value<Block> block);

	pipeline::Value<Segments> retrieveSegments(const Blocks& blocks);

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

	/**
	 * Generates a unique hash string for the given Segment. This is used as the primary key in the
	 * database.
	 */
	std::string generateHash(const boost::shared_ptr<Segment> segment);
	
	/**
	 * Get the hash for the given Segment. If this Segment has not already been seen, its hash will
	 * be generated and cached. If it has been seen, the hash will be retrieved from the cache and 
	 * returned.
	 */
	std::string getHash(const boost::shared_ptr<Segment> segment);
	
	/**
	 * Set the names of the features in the databse if they have not already been set.
	 */
	void setFeatureNames(const std::vector<std::string>& featureNames);
	
	/**
	 * A helper function to return the numerical type used in the database.
	 */
	static int getSegmentType(const boost::shared_ptr<Segment> segment);
	
	/**
	 * A helper function to return the numerical direction used in the database.
	 */
	static int getSegmentDirection(const boost::shared_ptr<Segment> segment);
	
	/**
	 * A helper function used to return the infimal section index.
	 */
	static unsigned int getSectionInfimum(const boost::shared_ptr<Segment> segment);
	
	/**
	 * Converts a ptree into a Segment.
	 */
	boost::shared_ptr<Segment> ptreeToSegment(const boost::property_tree::ptree& pt);
	
	const boost::shared_ptr<DjangoSliceStore> _sliceStore;
	const std::string _server;
	const unsigned int _project, _stack;
	
	/* Here, we have several maps
	 * 
	 * Segment->string hash value
	 * string hash value->Segment
	 * id->Segment
	 * 
	 * For the first mapping, we use an unordered_map over a custom hash function rather than
	 * std::map. This is because std::map uses the default < operator, which operates over id,
	 * and can't be guaranteed to be consistent in our case. Also, we need to use a pointer since
	 * Segment is abstract.
	 */
	boost::unordered_map<boost::shared_ptr<Segment>, std::string,
		SegmentPointerHash, SegmentPointerEquals> _segmentHashMap;
	std::map<std::string, boost::shared_ptr<Segment> > _hashSegmentMap;
	std::map<unsigned int, boost::shared_ptr<Segment> > _idSegmentMap;
	
	// Set to true if feature names have already been set.
	bool _featureNamesFlag;
};


#endif //DJANGO_SEGMENT_STORE_H__
