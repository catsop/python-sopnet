#ifndef SOPNET_HISTOGRAM_FEATURE_EXTRACTOR_H_
#define SOPNET_HISTOGRAM_FEATURE_EXTRACTOR_H_

#include <pipeline/all.h>
#include <imageprocessing/ImageStack.h>
#include <segments/Segments.h>
#include <features/Features.h>
#include <util/point.hpp>

class HistogramFeatureExtractor : public pipeline::SimpleProcessNode<> {

public:

	HistogramFeatureExtractor(unsigned int numBins);

private:

	void updateOutputs();

	void getFeatures(const EndSegment& end, std::vector<double>& slice);

	void getFeatures(const ContinuationSegment& continuation, std::vector<double>& slice);

	void getFeatures(const BranchSegment& branch, std::vector<double>& slice);

	std::vector<double> computeHistogram(const Slice& slice);

	pipeline::Input<Segments> _segments;

	pipeline::Input<ImageStack<IntensityImage> > _sections;
	
	pipeline::Input<util::point<unsigned int, 3> > _cropOffset;

	pipeline::Output<Features> _features;

	unsigned int _numBins;
};

#endif // SOPNET_HISTOGRAM_FEATURE_EXTRACTOR_H_

