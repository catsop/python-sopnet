#ifndef LOCAL_SEGMENT_STORE_H__
#define LOCAL_SEGMENT_STORE_H__

#include <catmaid/persistence/SegmentStore.h>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/shared_ptr.hpp>
#include <sopnet/segments/Segment.h>
#include <sopnet/segments/Segments.h>
#include <catmaid/blocks/Block.h>
#include <catmaid/blocks/Core.h>
#include <catmaid/blocks/Blocks.h>
#include <sopnet/segments/SegmentSet.h>
#include <catmaid/persistence/SegmentPointerHash.h>


class LocalSegmentStore : public SegmentStore
{
	typedef boost::unordered_map<boost::shared_ptr<Segment>, boost::shared_ptr<Blocks>,
		SegmentPointerHash, SegmentPointerEquals > SegmentBlockMap;
	typedef boost::unordered_map<Block, boost::shared_ptr<Segments> > BlockSegmentMap;
	typedef boost::unordered_map<unsigned int, boost::shared_ptr<Segment> > IdSegmentMap;
	typedef boost::unordered_map<boost::shared_ptr<Segment>, double,
		SegmentPointerHash, SegmentPointerEquals > SegmentCostMap;
	typedef boost::unordered_map<Core, SegmentCostMap> SegmentSolutionMap;
	
public:

	LocalSegmentStore();

	/**
	 * Associate a set of segment descritptions to a block. A "descritption" is 
	 * a SegmentDescritption that represents a segment only by its hash, 
	 * features, and slice hashes.
	 *
	 * @param segments
	 *              A description of the segments that are supposed to be stored 
	 *              in the database.
	 * @param block
	 *              The block to which to associate the segments.
	 */
	void associateSegmentsToBlock(
			const SegmentDescriptions& segments,
			const Block&               block) {}

	/**
	 * Get a description of all the segments in the given blocks. A 
	 * "descritption" is a SegmentDescritption that represents a segment only by 
	 * its hash, features, and slice hashes.
	 *
	 * @param blocks
	 *              The blocks from which to retrieve the segments.
	 *
	 * @param missingBlocks
	 *              A reference to a block collection. This collection will be 
	 *              filled with the blocks for which no segments are available, 
	 *              yet. This collection will be empty on success.
	 */
	boost::shared_ptr<SegmentDescriptions> getSegmentsByBlocks(
			const Blocks& blocks,
			Blocks&       missingBlocks) {}

	/**
	 * Store the solution of processing a core.
	 *
	 * @param segmentHashes
	 *              A list of segment hashes that are part of the solution. A 
	 *              concrete implementation has to make sure that all other 
	 *              segments associated to this core are marked as not belonging 
	 *              to the solution.
	 * @param core
	 *              The core for which the solution was generated.
	 */
	void storeSolution(const std::vector<std::size_t>& segmentHashes, const Core& core) {}


	/******************************************
	 * OLD INTERFACE DEFINITION -- DEPRECATED *
	 ******************************************/
	
    /**
     * Associates a segment with a block
     * @param segment - the segment to store.
     * @param block - the block containing the segment.
     */
    void associate(pipeline::Value<Segments> segmentsIn,
				   pipeline::Value<Block> block);

    /**
     * Retrieve all segments that are at least partially contained in the given block.
     * @param block - the Block for which to retrieve all segments.
     */
    pipeline::Value<Segments> retrieveSegments(const Blocks& blocks);

	pipeline::Value<Blocks> getAssociatedBlocks(pipeline::Value<Segment> segment);
	
	void dumpStore();
	
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
	
private:
	void mapSegmentToBlock(const boost::shared_ptr<Segment>& segment,
						   const boost::shared_ptr<Block>& block);

	
	void mapBlockToSegment(const boost::shared_ptr<Block>& block,
						   const boost::shared_ptr<Segment>& segment);

	void addSegmentToMasterList(const boost::shared_ptr<Segment>& segment);
	
	boost::shared_ptr<Segment> equivalentSegment(const boost::shared_ptr<Segment>& segment);
	
	boost::shared_ptr<Segments> getSegments(const boost::shared_ptr<Block>& block,
		const boost::shared_ptr<Segment>& segment);
	
	static bool compareSegments(const boost::shared_ptr<Segment> segment1,
								const boost::shared_ptr<Segment> segment2)
	{
		return segment1->getId() < segment2->getId();
	}
	
	SegmentBlockMap _segmentBlockMap;
	BlockSegmentMap _blockSegmentMap;
	IdSegmentMap _idSegmentMap;
	
	SegmentFeaturesMap _featureMasterMap;
	SegmentCostMap _costMap;
	SegmentSolutionMap _solutionMap;
	SegmentSet _segmentMasterList;
	
	std::vector<std::string> _featureNames;

};

#endif //LOCAL_SEGMENT_STORE_H__
