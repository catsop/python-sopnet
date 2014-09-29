#ifndef SEGMENT_STORE_H__
#define SEGMENT_STORE_H__

#include <boost/unordered_map.hpp>

#include <catmaid/blocks/Block.h>
#include <catmaid/blocks/Blocks.h>
#include <catmaid/blocks/Core.h>
#include <catmaid/persistence/SegmentDescriptions.h>

// TODO: remove when new API completely implemented
#include <catmaid/persistence/SegmentPointerHash.h>
#include <sopnet/segments/Segments.h>
#include <sopnet/features/Features.h>
#include <sopnet/inference/Solution.h>
#include <sopnet/inference/LinearObjective.h>
#include <pipeline/Value.h>


/**
 * Segment store interface definition.
 */
class SegmentStore {

public:

	/**
	 * Associate a set of segment descritptions to a block. A "descritption" is 
	 * a SegmentDescription that represents a segment only by its hash, 
	 * features, and slice hashes.
	 *
	 * @param segments
	 *              A description of the segments that are supposed to be stored 
	 *              in the database.
	 * @param block
	 *              The block to which to associate the segments.
	 */
	virtual void associateSegmentsToBlock(
			const SegmentDescriptions& segments,
			const Block&               block) = 0;

	/**
	 * Get a description of all the segments in the given blocks. A 
	 * "descritption" is a SegmentDescription that represents a segment only by 
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
	virtual boost::shared_ptr<SegmentDescriptions> getSegmentsByBlocks(
			const Blocks& blocks,
			Blocks&       missingBlocks) = 0;

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
	virtual void storeSolution(const std::vector<std::size_t>& segmentHashes, const Core& core) = 0;


	/******************************************
	 * OLD INTERFACE DEFINITION -- DEPRECATED *
	 ******************************************/

	typedef boost::unordered_map<boost::shared_ptr<Segment>,
								 std::vector<double>,
								 SegmentPointerHash,
								 SegmentPointerEquals> SegmentFeaturesMap;

	/**
	 * Write Segments over the given Blocks to this SegmentStore.
	 */
	DEPRECATED(void writeSegments(const Segments& segments, const Blocks& blocks)) {}

    /**
     * Associates a segment with a block
     * @param segment - the segment to store.
     * @param block - the block containing the segment.
     */
    DEPRECATED(virtual void associate(pipeline::Value<Segments> segments,
						   pipeline::Value<Block> block)) = 0;

    /**
     * Retrieve all segments that are at least partially contained in the given block.
     * @param block - the Block for which to retrieve all segments.
     */
    DEPRECATED(virtual pipeline::Value<Segments> retrieveSegments(const Blocks& blocks)) = 0;

	/**
	 * Retrieve a set of Blocks that are associated with the given Segment.
	 * @param segment - the Segment for which to retrieve associated Blocks
	 */
	DEPRECATED(virtual pipeline::Value<Blocks> getAssociatedBlocks(pipeline::Value<Segment> segment)) = 0;
	
	/**
	 * Store a set of Features associated to a set of Segments. Segments must be associated before
	 * Features may be associated with them.
	 * @param features - the Features to store
	 * @return the number of Segments for which Features were stored. For instance, if this is not
	 * equal to the size of features, then there were Segments for which Features were calculated
	 * but not stored. This may not necessarily be an error, however.
	 */
	DEPRECATED(virtual int storeFeatures(pipeline::Value<Features> features)) = 0;

	/**
	 * Retrieve a set of Feautres associated to the set of given Segments.
	 * @param segments - the Segments for which to retrieve Features
	 * @return a map from a shared_ptr to a Segment to a vector of double values representing the
	 * features for that Segment.
	 * 
	 * Conversion to a Features object is handled in SegmentFeatureReader.
	 */
	DEPRECATED(virtual pipeline::Value<SegmentFeaturesMap>
		retrieveFeatures(pipeline::Value<Segments> segments)) = 0;

	/**
	 * Retrieve the names of the Segment Features that have been stored herel.
	 */
	DEPRECATED(virtual std::vector<std::string> getFeatureNames()) = 0;

	/**
	 * Store costs for the given segments.
	 * @param segments - the Segments corresponding to the given Solution object
	 * @param objective - the LinearObjective, which contains costs assigned by, for instance, a
	 * LinearCostFunction (as generated by the ObjectiveGenerator)
	 * @return the number of Segments for which the costs were successfully stored. This will
	 * be less than the size of segments when not all of the Segments have already been associated
	 * to a block in this store.
	 * 
	 * An Object object stores a linear index of scores on the Segments that were given to the
	 * solver. In other words, objective->getCoefficients[i] returns the score corresponding to
	 * segments->getSegments()[i], so it is important to input the same Segments object that was used
	 * for the ObjectiveGenerator.
	 */
	DEPRECATED(virtual unsigned int storeCost(pipeline::Value<Segments> segments,
							   pipeline::Value<LinearObjective> objective)) = 0;

	/**
	 * Retrieve the costs for the given segments.
	 * @param segments - the Segments for which solution scores are to be retrieved.
	 * @param defaultCost - the cost to assign to segments for which there is no stored
	 * cost
	 * @param segmentsNF - an empty Segments object, into which any Segment that does not have
	 * a stored cost will be added.
	 * @return the corresponding LinearObjective object
	 * 
	 * The returned LinearObjective object will be indexed to correspond to the segments
	 * argument. Segments for which there is no cost stored in this SegmentStore will be
	 * assigned the defaultCost value. 
	 */
	DEPRECATED(virtual pipeline::Value<LinearObjective> retrieveCost(pipeline::Value<Segments> segments,
														  double defaultCost,
														  pipeline::Value<Segments> segmentsNF)) = 0;

	/**
	 * Store the solution for a given set of Segments and a give Core.
	 * @param segments - the segments for which the Solution is to be stored
	 * @param core - the Core to which this segments-solution assignment is to be associated.
	 * @param solution - the solution to be assigned
	 * @param indices - a vector that maps indices from segments to solution
	 * @return the number of Segments for which a solution was successfully stored. This will
	 * be less than the size of segments when not all of the Segments have already been associated
	 * to a Block in the given Core.
	 * 
	 * A Solution is indexed linearly to a Segments object when it is created, but we usually
	 * will want to store solutions for a subset of given segments. The indices vector maps from
	 * indices in segments to indices in solution, in other words:
	 * let j = indices[i], then the ith segment maps to the jth value in solution
	 */
	DEPRECATED(virtual unsigned int storeSolution(pipeline::Value<Segments> segments,
									   pipeline::Value<Core> core,
									   pipeline::Value<Solution> solution,
									   std::vector<unsigned int> indices)) = 0;

	/**
	 * Retrieve the solutions for the given segments, associated with the given Core.
	 * @param segments - the Segments for which solution scores are to be retrieved.
	 * @param core - the Core for which the solution was generated.
	 * @return the corresponding LinearObjective object
	 * 
	 * The returned Solution object will be indexed to correspond to the segments
	 * argument. Any Segments not associated to a Block in core, or without an associated solution
	 * value in the store will be assigned a solution value of 0.
	 */
	DEPRECATED(virtual pipeline::Value<Solution> retrieveSolution(pipeline::Value<Segments> segments,
													   pipeline::Value<Core> core)) = 0;
};


#endif //SEGMENT_STORE_H__
