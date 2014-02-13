#ifndef SEGMENT_STORE_H__
#define SEGMENT_STORE_H__

#include <sopnet/segments/Segment.h>
#include <sopnet/segments/Segments.h>
#include <sopnet/block/Block.h>
#include <sopnet/block/Blocks.h>
#include <pipeline/Data.h>
#include <pipeline/Value.h>

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
    virtual void associate(pipeline::Value<Segments> segments,
						   pipeline::Value<Block> block) = 0;

    /**
     * Retrieve all segments that are at least partially contained in the given block.
     * @param block - the Block for which to retrieve all segments.
     */
    virtual pipeline::Value<Segments> retrieveSegments(pipeline::Value<Blocks> blocks) = 0;

	virtual pipeline::Value<Blocks> getAssociatedBlocks(pipeline::Value<Segment> segment) = 0;

};


#endif //SEGMENT_STORE_H__