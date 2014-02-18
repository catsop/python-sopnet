#ifndef LOCAL_SEGMENT_STORE_H__
#define LOCAL_SEGMENT_STORE_H__

#include <catmaid/persistence/SegmentStore.h>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <boost/shared_ptr.hpp>
#include <sopnet/segments/Segment.h>
#include <sopnet/segments/Segments.h>
#include <sopnet/block/Block.h>
#include <sopnet/block/Blocks.h>
#include "SegmentPointerHash.h"


class LocalSegmentStore : public SegmentStore
{
	typedef boost::unordered_map<boost::shared_ptr<Segment>, boost::shared_ptr<Blocks>,
		SegmentPointerHash, SegmentPointerEquals > SegmentBlockMap;
	typedef boost::unordered_map<Block, boost::shared_ptr<Segments> > BlockSegmentMap;
	typedef boost::unordered_map<unsigned int, boost::shared_ptr<Segment> > IdSegmentMap;
	
public:
	LocalSegmentStore();
	
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
    pipeline::Value<Segments> retrieveSegments(pipeline::Value<Blocks> blocks);

	pipeline::Value<Blocks> getAssociatedBlocks(pipeline::Value<Segment> segment);
	
	void dumpStore();
	
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
	
	SegmentSet _segmentMasterList;

};

#endif //LOCAL_SEGMENT_STORE_H__
