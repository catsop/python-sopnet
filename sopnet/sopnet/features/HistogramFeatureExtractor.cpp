#include <boost/lexical_cast.hpp>

#include <imageprocessing/ConnectedComponent.h>
#include <sopnet/segments/EndSegment.h>
#include <sopnet/segments/ContinuationSegment.h>
#include <sopnet/segments/BranchSegment.h>
#include "HistogramFeatureExtractor.h"

#include <util/Logger.h>

logger::LogChannel histogramfeaturelog("histogramfeaturelog", "[HistogramFeature] ");

HistogramFeatureExtractor::HistogramFeatureExtractor(unsigned int numBins) :
	_features(boost::make_shared<Features>()),
	_numBins(numBins) {

	registerInput(_segments, "segments");
	registerInput(_sections, "raw sections");
	registerInput(_cropOffset, "crop offset", pipeline::Optional);
	registerOutput(_features, "features");
}

void
HistogramFeatureExtractor::updateOutputs() {

	_features->clear();
	
	LOG_DEBUG(histogramfeaturelog) << "Got images of size" << (*_sections)[0]->width() << " x " <<
		(*_sections)[0]->height() << std::endl;

	for (unsigned int i = 0; i < _numBins; i++)
		_features->addName("histogram " + boost::lexical_cast<std::string>(i));

	for (unsigned int i = 0; i < _numBins; i++)
		_features->addName("normalized histogram " + boost::lexical_cast<std::string>(i));

	_features->resize(_segments->size(), 2*_numBins);
	
	LOG_ALL(histogramfeaturelog) << "Features names set up, now lets extract them" << std::endl;

	foreach (boost::shared_ptr<EndSegment> segment, _segments->getEnds())
		getFeatures(*segment, _features->get(segment->getId()));

	foreach (boost::shared_ptr<ContinuationSegment> segment, _segments->getContinuations())
		getFeatures(*segment, _features->get(segment->getId()));

	foreach (boost::shared_ptr<BranchSegment> segment, _segments->getBranches())
		getFeatures(*segment, _features->get(segment->getId()));
}

void
HistogramFeatureExtractor::getFeatures(const EndSegment& end, std::vector<double>& features) {

	std::vector<double> histogram = computeHistogram(*end.getSlice().get());

	for (unsigned int i = 0; i < _numBins; i++)
		features[i] = histogram[i];

	double sum = 0;
	for (unsigned int i = 0; i < _numBins; i++)
		sum += histogram[i];

	for (unsigned int i = 0; i < _numBins; i++)
		features[_numBins + i] = histogram[i]/sum;
}

void
HistogramFeatureExtractor::getFeatures(const ContinuationSegment& continuation, std::vector<double>& features) {

	std::vector<double> sourceHistogram = computeHistogram(*continuation.getSourceSlice().get());
	std::vector<double> targetHistogram = computeHistogram(*continuation.getTargetSlice().get());

	for (unsigned int i = 0; i < _numBins; i++)
		features[i] = std::abs(sourceHistogram[i] - targetHistogram[i]);

	double sourceSum = 0;
	double targetSum = 0;
	for (unsigned int i = 0; i < _numBins; i++) {

		sourceSum += sourceHistogram[i];
		targetSum += targetHistogram[i];
	}

	for (unsigned int i = 0; i < _numBins; i++)
		features[_numBins + i] = std::abs(sourceHistogram[i]/sourceSum - targetHistogram[i]/targetSum);
}

void
HistogramFeatureExtractor::getFeatures(const BranchSegment& branch, std::vector<double>& features) {

	std::vector<double> sourceHistogram  = computeHistogram(*branch.getSourceSlice().get());
	std::vector<double> targetHistogram1 = computeHistogram(*branch.getTargetSlice1().get());
	std::vector<double> targetHistogram2 = computeHistogram(*branch.getTargetSlice2().get());

	std::vector<double> targetHistogram = targetHistogram1;

	for (unsigned int i = 0; i < _numBins; i++)
		targetHistogram[i] += targetHistogram2[i];

	for (unsigned int i = 0; i < _numBins; i++)
		features[i] = std::abs(sourceHistogram[i] - targetHistogram[i]);

	double sourceSum = 0;
	double targetSum = 0;
	for (unsigned int i = 0; i < _numBins; i++) {

		sourceSum += sourceHistogram[i];
		targetSum += targetHistogram[i];
	}

	for (unsigned int i = 0; i < _numBins; i++)
		features[_numBins + i] = std::abs(sourceHistogram[i]/sourceSum - targetHistogram[i]/targetSum);
}

std::vector<double>
HistogramFeatureExtractor::computeHistogram(const Slice& slice) {

	util::point3<unsigned int> offset = _cropOffset ? *_cropOffset : util::point3<unsigned int>(0, 0, 0);
	util::point<unsigned int> offset2D = offset;

	std::vector<double> histogram(_numBins, 0);

	unsigned int section = slice.getSection() - offset.z;

	Image& image = *(*_sections)[section];
	
	LOG_ALL(histogramfeaturelog) << "Offset:      " << offset << std::endl;
	LOG_ALL(histogramfeaturelog) << "Image size:  " << image.width() << "x" << image.height() <<
		std::endl;
	LOG_ALL(histogramfeaturelog) << "Slice bound: " << slice.getComponent()->getBoundingBox() <<
		std::endl;

	foreach (const util::point<unsigned int>& pixel, slice.getComponent()->getPixels()) {

		double value = image(pixel.x - offset.x, pixel.y - offset.y);

		unsigned int bin = std::min(_numBins - 1, (unsigned int)(value*_numBins));

		histogram[bin]++;
	}
	
	return histogram;
}
