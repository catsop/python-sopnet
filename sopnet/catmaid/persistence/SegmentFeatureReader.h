#ifndef SEGMENT_FEATURE_READER_H__
#define SEGMENT_FEATURE_READER_H__

#include <pipeline/all.h>
#include <catmaid/persistence/SegmentStore.h>
#include <sopnet/segments/Segments.h>
#include <sopnet/features/Features.h>
#include <sopnet/block/BlockManager.h>
#include <catmaid/persistence/StackStore.h>

class SegmentFeatureReader : public pipeline::SimpleProcessNode<>
{
public:
	/**
	 * Construct a SegmentFeatureReader, which is a ProcessNode that
	 * retrieves Features from the SegmentStore for the given set of
	 * Segments. Optionally, Features will be generated for the Segments
	 * that have no corresponding Features in the store.
	 * 
	 * Inputs:
	 *  Segments     "segments"
	 *  SegmentStore "store"
	 *  bool         "stored only" - optional, defaults to false
	 *  BlockManager "block manager" - optional, but needed if "stored only" is false
	 *  StackStore   "raw stack store - optional, but needed if "stored only" is false
	 * 
	 * Outputs:
	 *  Features "features"
	 */
    SegmentFeatureReader();
	
private:
	void updateOutputs();
	
	/**
	 * Reconstitutes a Features object from the SegmentFeatureMap created by
	 * the SegmentStore. featurelessSegments, if non-null, will be populated
	 * with the Segments for which there were no features.
	 */
	boost::shared_ptr<Features> reconstituteFeatures(
		const boost::shared_ptr<Segments> featurelessSegments);
	
	/**
	 * Extract Features from the given Segments, and combine them with storedFeatures.
	 */
	boost::shared_ptr<Features> assembleFeatures(
		const boost::shared_ptr<Features> storedFeatures,
		const boost::shared_ptr<Segments> segments);
	
	/**
	 * Extract Features for Segments that don't have stored ones.
	 */
	boost::shared_ptr<Features> extractFeatures(const boost::shared_ptr<Segments> segments,
												const boost::shared_ptr<Blocks> boundingBlocks);
	
	/**
	 * Helper function to assembleFeatures that appends inFeatures to outFeatures.
	 */
	void appendFeatures(const boost::shared_ptr<Features> outFeatures,
						std::map<unsigned int, unsigned int>& outSegmentIdsMap,
						const boost::shared_ptr<Features> inFeatures);
	
	pipeline::Input<Segments> _segments;
	pipeline::Input<SegmentStore> _store;
	pipeline::Input<bool> _storedOnly;
	pipeline::Input<BlockManager> _blockManager;
	pipeline::Input<StackStore> _rawStackStore;
	
	pipeline::Output<Features> _features;
};

#endif //SEGMENT_FEATURE_READER_H__