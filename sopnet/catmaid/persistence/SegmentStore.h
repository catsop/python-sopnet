#ifndef SEGMENT_STORE_H__
#define SEGMENT_STORE_H__

#include <sopnet/segments/Segment.h>
#include <sopnet/segments/Segments.h>
#include <sopnet/block/Block.h>
#include <sopnet/block/Blocks.h>
#include <pipeline/Data.h>

class SegmentStoreResult : public pipeline::Data
{
public:
	SegmentStoreResult() : count(0) {}
	
	int count;
};

/**
 * Abstract Data class that handles the practicalities of storing and retrieving Segments from a store.
 */
class SegmentStore : public pipeline::Data
{
public:
    /**
     * Associates a segment with a block
     * @param segment - the segment to store.
     * @param block - the block containing the segment.
     */
    virtual void associate(const boost::shared_ptr<Segment>& segment, const boost::shared_ptr<Block>& block) = 0;

    /**
     * Retrieve all segments that are at least partially contained in the given block.
     * @param block - the Block for which to retrieve all segments.
     */
    virtual boost::shared_ptr<Segments> retrieveSegments(const boost::shared_ptr<Block>& block) = 0;

	virtual void disassociate(const boost::shared_ptr<Segment>& segment, const boost::shared_ptr<Block>& block) = 0;

	virtual void removeSegment(const boost::shared_ptr<Segment>& segments) = 0;

	virtual boost::shared_ptr<Blocks> getAssociatedBlocks(const boost::shared_ptr<Segment>& segment) = 0;

};


#endif //SEGMENT_STORE_H__