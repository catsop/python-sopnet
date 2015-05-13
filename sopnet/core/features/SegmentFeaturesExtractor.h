#ifndef SOPNET_SEGMENT_FEATURES_EXTRACTOR_H__
#define SOPNET_SEGMENT_FEATURES_EXTRACTOR_H__

#include <pipeline/all.h>
#include <imageprocessing/ImageStack.h>
#include <sopnet/segments/Segments.h>
#include <util/point.hpp>
#include "Features.h"

// forward declaration
class GeometryFeatureExtractor;
class HistogramFeatureExtractor;
class TypeFeatureExtractor;

class SegmentFeaturesExtractor : public pipeline::ProcessNode {

public:

	/**
	 * Construct a SegmentFeaturesExtractor
	 * Inputs:
	 *   Segments "segments" - the Segments for which Features are to be extracted
	 *   ImageStack "raw sections" - raw images representing the sections
	 *   util::point<unsigned int, 3> "crop offset" - optional - points to the offset of the stack
	 *                               crop, in the case that the stack has been cropped before
	 *                               features are to be extracted.
	 * 
	 * Outputs:
	 *  Features "all features" - the Features extracted from "segments" 
	 */
	SegmentFeaturesExtractor();

private:

	class FeaturesAssembler : public pipeline::SimpleProcessNode<> {

	public:

		FeaturesAssembler();

	private:

		void updateOutputs();

		pipeline::Inputs<Features> _features;

		pipeline::Output<Features> _allFeatures;
	};

	void onInputSet(const pipeline::InputSetBase& signal);
	
	void onOffsetSet(const pipeline::InputSetBase&);

	pipeline::Input<Segments> _segments;

	pipeline::Input<ImageStack> _rawSections;
	
	pipeline::Input<util::point<unsigned int, 3> > _cropOffset;

	boost::shared_ptr<GeometryFeatureExtractor>  _geometryFeatureExtractor;

	boost::shared_ptr<HistogramFeatureExtractor> _histogramFeatureExtractor;

	boost::shared_ptr<TypeFeatureExtractor> _typeFeatureExtractor;

	boost::shared_ptr<FeaturesAssembler> _featuresAssembler;
};

#endif // SOPNET_SEGMENT_FEATURES_EXTRACTOR_H__

