#ifndef SEGMENT_STORE_H__
#define SEGMENT_STORE_H__

#include <boost/unordered_map.hpp>

#include <sopnet/segments/Segment.h>
#include <sopnet/segments/Segments.h>
#include <sopnet/block/Block.h>
#include <sopnet/block/Blocks.h>
#include <pipeline/Data.h>
#include <pipeline/Value.h>
#include <sopnet/features/Features.h>
#include <sopnet/inference/Solution.h>
#include <catmaid/persistence/SegmentPointerHash.h>

/**
 * Abstract Data class that handles the practicalities of storing and retrieving Segments from a store.
 */
class SegmentStore : public pipeline::Data
{
public:
	typedef boost::unordered_map<boost::shared_ptr<Segment>,
								 std::vector<double>,
								 SegmentPointerHash,
								 SegmentPointerEquals> SegmentFeaturesMap;
	
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

	/**
	 * Retrieve a set of Blocks that are associated with the given Segment.
	 * @param segment - the Segment for which to retrieve associated Blocks
	 */
	virtual pipeline::Value<Blocks> getAssociatedBlocks(pipeline::Value<Segment> segment) = 0;
	
	/**
	 * Store a set of Features associated to a set of Segments. Segments must be associated before
	 * Features may be associated with them.
	 * @param features - the Features to store
	 * @return the number of Segments for which Features were stored. For instance, if this is not
	 * equal to the size of features, then there were Segments for which Features were calculated
	 * but not stored. This may not necessarily be an error, however.
	 */
	virtual int storeFeatures(pipeline::Value<Features> features) = 0;

	/**
	 * Retrieve a set of Feautres associated to the set of given Segments.
	 * @param segments - the Segments for which to retrieve Features
	 * @return a map from a shared_ptr to a Segment to a vector of double values representing the
	 * features for that Segment.
	 * 
	 * Conversion to a Features object is handled in SegmentFeatureReader.
	 */
	virtual pipeline::Value<SegmentFeaturesMap>
		retrieveFeatures(pipeline::Value<Segments> segments) = 0;

	/**
	 * Retrieve the names of the Segment Features that have been stored herel.
	 */
	virtual std::vector<std::string> getFeatureNames() = 0;

	/**
	 * Store solution scores for the given segments.
	 * @param segments - the Segments corresponding to the given Solution object
	 * @param solution - the Solution scores, as computed for instance by LinearSolver
	 * @return the number of Segments for which the scores were successfully stored. This will
	 * be less than the size of segments when not all of the Segments have already been associated.
	 * 
	 * A Solution object stores a linear index of scores on the Segments that were given to the
	 * solver. In other words, (*solution)[i] returns the score corresponding to
	 * segments->getSegments()[i], so it is important to input the same Segments object that was used
	 * for the Solver.
	 */
	virtual unsigned int storeSolution(pipeline::Value<Segments> segments,
							   pipeline::Value<Solution> solution) = 0;

	/**
	 * Retrieve the solution scores for the given segments.
	 * @param segments - the Segments for which solution scores are to be retrieved.
	 * @return the corresponding Solution object
	 * 
	 * The returned Solution object will be indexed to correspond to the segments argument. The
	 * score for a Segment will be zero in the case that it is not associated in this store, or no
	 * corresponding score has been stored.
	 */
	virtual pipeline::Value<Solution> retrieveSolution(pipeline::Value<Segments> segments) = 0;
	
	/**
	 * Print the contents of this store to the DEBUG logging channel.
	 */
	virtual void dumpStore() = 0;
};


#endif //SEGMENT_STORE_H__