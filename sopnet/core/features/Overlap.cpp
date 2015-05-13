#include <imageprocessing/ConnectedComponent.h>
#include <util/box.hpp>
#include <sopnet/slices/Slice.h>
#include "Overlap.h"

double
Overlap::operator()(const Slice& slice1, const Slice& slice2) {

	// values to add to slice2's pixel positions
	util::point<int, 2> offset2(0, 0);

	// ...only non-zero if we want to align both slices
	if (_align)
		offset2 = slice1.getComponent()->getCenter() - slice2.getComponent()->getCenter();

	unsigned int numOverlap = overlap(
			*slice1.getComponent(),
			*slice2.getComponent(),
			offset2);

	if (_normalized) {

		return normalize(slice1, slice2, numOverlap);

	} else {

		return numOverlap;
	}
}

double
Overlap::operator()(const Slice& slice1a, const Slice& slice1b, const Slice& slice2) {

	// values to add to slice2's pixel positions
	util::point<int, 2> offset2(0, 0);

	// ...only non-zero if we want to align slice2 to both slice1s
	if (_align) {

		// the mean pixel location of slice1a and slice1b
		util::point<double, 2> center1 = 
				(slice1a.getComponent()->getCenter()*slice1a.getComponent()->getSize()
				 +
				 slice1b.getComponent()->getCenter()*slice1b.getComponent()->getSize())
				/
				(double)(slice1a.getComponent()->getSize() + slice1b.getComponent()->getSize());

		offset2 = center1 - slice2.getComponent()->getCenter();
	}

	unsigned int numOverlapa = overlap(
			*slice1a.getComponent(),
			*slice2.getComponent(),
			offset2);
	unsigned int numOverlapb = overlap(
			*slice1b.getComponent(),
			*slice2.getComponent(),
			offset2);

	unsigned int numOverlap = numOverlapa + numOverlapb;

	if (_normalized) {

		return normalize(slice1a, slice1b, slice2, numOverlap);

	} else {

		return numOverlap;
	}
}

bool
Overlap::exceeds(const Slice& slice1, const Slice& slice2, double value) {

	double _;
	return exceeds(slice1, slice2, value, _);
}

bool
Overlap::exceeds(const Slice& slice1, const Slice& slice2, double value, double& exceededValue) {

	/**
	 * First, compute the upper bound for the slice overlap based on the 
	 * bounding boxes.
	 */

	// values to add to slice2's pixel positions
	util::point<int, 2> offset2(0, 0);

	// ...only non-zero if we want to align both slices
	if (_align)
		offset2 = slice1.getComponent()->getCenter() - slice2.getComponent()->getCenter();

	util::box<double, 2> bb_intersection = slice1.getComponent()->getBoundingBox().intersection(slice2.getComponent()->getBoundingBox() + offset2);

	double maxOverlap = bb_intersection.area();

	if (_normalized)
		maxOverlap = normalize(slice1, slice2, maxOverlap);

	/**
	 * If this exceeds the threshold, perform the exact computation of the 
	 * overlap.
	 */

	if (maxOverlap <= value) {

		exceededValue = value;
		return false;
	}

	exceededValue = (*this)(slice1, slice2);

	return exceededValue > value;
}

bool
Overlap::exceeds(const Slice& slice1a, const Slice& slice1b, const Slice& slice2, double value) {

	/**
	 * First, compute the upper bound for the slice overlap based on the 
	 * bounding boxes.
	 */

	// values to add to slice2's pixel positions
	util::point<int, 2> offset2(0, 0);

	// ...only non-zero if we want to align slice2 to both slice1s
	if (_align) {

		// the mean pixel location of slice1a and slice1b
		util::point<double, 2> center1 = 
				(slice1a.getComponent()->getCenter()*slice1a.getComponent()->getSize()
				 +
				 slice1b.getComponent()->getCenter()*slice1b.getComponent()->getSize())
				/
				(double)(slice1a.getComponent()->getSize() + slice1b.getComponent()->getSize());

		offset2 = center1 - slice2.getComponent()->getCenter();
	}

	util::box<double, 2> bb_intersection_a = slice1a.getComponent()->getBoundingBox().intersection(slice2.getComponent()->getBoundingBox() + offset2);
	util::box<double, 2> bb_intersection_b = slice1b.getComponent()->getBoundingBox().intersection(slice2.getComponent()->getBoundingBox() + offset2);

	double maxOverlap = bb_intersection_a.area() + bb_intersection_b.area();

	if (_normalized)
		maxOverlap = normalize(slice1a, slice1b, slice2, maxOverlap);

	/**
	 * If this exceeds the threshold, perform the exact computation of the 
	 * overlap.
	 */

	if (maxOverlap <= value)
		return false;

	return (*this)(slice1a, slice1b, slice2) > value;
}

unsigned int
Overlap::overlap(
		const ConnectedComponent& c1,
		const ConnectedComponent& c2,
		const util::point<int, 2>& offset2) {

	if (!c1.getBoundingBox().intersects(c2.getBoundingBox() + offset2))
		return 0;

	unsigned int numOverlap = 0;

	const ConnectedComponent& smaller = (c1 < c2 ? c1 : c2);
	const ConnectedComponent& bigger  = (c1 < c2 ? c2 : c1);

	const ConnectedComponent::bitmap_type& biggerBitmap = (c1 < c2 ? c2.getBitmap() : c1.getBitmap());

	// the offset from the smaller component to the bigger component
	util::point<int, 2> smallerToBigger = (c1 < c2 ? -offset2 : offset2);

	// the same, but to the pixel positions in the bigger component's bitmap
	util::point<int, 2> toBitmap = smallerToBigger - util::point<int, 2>(bigger.getBoundingBox().min().x(), bigger.getBoundingBox().min().y());

	// width and height of the bigger bounding box
	int width  = bigger.getBoundingBox().width();
	int height = bigger.getBoundingBox().height();

	// iterate over all pixels in the smaller component
	for (const util::point<unsigned int, 2>& pixel : smaller.getPixels()) {

		// add offset from smaller to bigger pixel positions in the bitmap
		util::point<int, 2> inBitmap = util::point<int, 2>(pixel) + toBitmap;

		if (inBitmap.x() >= 0 && inBitmap.y() >= 0 && inBitmap.x() < width && inBitmap.y() < height)
			if (biggerBitmap(inBitmap.x(), inBitmap.y()))
				numOverlap++;
	}

	return numOverlap;
}

double
Overlap::normalize(const Slice& slice1, const Slice& slice2, unsigned int overlap) {

	int totalSize = slice1.getComponent()->getSize() + slice2.getComponent()->getSize() - overlap;

	if (totalSize <= 0)
		totalSize = 1;

	return static_cast<double>(overlap)/totalSize;
}

double
Overlap::normalize(const Slice& slice1a, const Slice& slice1b, const Slice& slice2, unsigned int overlap) {

	int totalSize = slice1a.getComponent()->getSize() + slice1b.getComponent()->getSize() + slice2.getComponent()->getSize() - overlap;

	if (totalSize <= 0)
		totalSize = 1;

	return static_cast<double>(overlap)/totalSize;
}
