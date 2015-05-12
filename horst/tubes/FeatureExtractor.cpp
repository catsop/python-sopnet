#include <region_features/RegionFeatures.h>
#include <util/timing.h>
#include "FeatureExtractor.h"

void
FeatureExtractor::extractFrom(
		const ExplicitVolume<float>& intensities,
		const ExplicitVolume<int>&   labels) {

	UTIL_ASSERT_REL(intensities.data().shape(), ==, labels.data().shape());
	UTIL_ASSERT_REL(intensities.getBoundingBox(), ==, labels.getBoundingBox());
	UTIL_ASSERT_REL(intensities.getResolutionX(), ==, labels.getResolutionX());
	UTIL_ASSERT_REL(intensities.getResolutionY(), ==, labels.getResolutionY());
	UTIL_ASSERT_REL(intensities.getResolutionZ(), ==, labels.getResolutionZ());

	UTIL_TIME_METHOD;

	RegionFeatures<3, float, int> regionFeatures(intensities.data(), labels.data());

	// Here we assume that the values in the labels volume match the region 
	// ids (which is true, for now).
	Features features;
	regionFeatures.fill(features);

	_store->saveFeatures(features);
	_store->saveFeatureNames(regionFeatures.getFeatureNames());
}

void
FeatureExtractor::extractFrom(
		const ExplicitVolume<float>& /*intensities*/,
		const Volumes&               /*volumes*/) {

	UTIL_THROW_EXCEPTION(
			NotYetImplemented,
			"");
}
