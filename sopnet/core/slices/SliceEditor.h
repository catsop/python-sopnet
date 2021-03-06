#ifndef SOPNET_SLICES_SLICE_EDITOR_H__
#define SOPNET_SLICES_SLICE_EDITOR_H__

#include <vector>
#include <boost/shared_ptr.hpp>
#include <imageprocessing/Image.h>
#include <util/point.hpp>
#include <util/box.hpp>
#include <slices/Slice.h>
#include <slices/SliceEdits.h>

/**
 * Editor to modify a set of slices on the pixel level.
 */
class SliceEditor {

public:

	/**
	 * Create a new slice editor for the given set of slices.
	 *
	 * @param initialSlices
	 *              Set of initial slices.
	 * @param section
	 *              Section number to use for new slices.
	 * @param region
	 *              The edit region in pixel coordinats. Should fit at least the 
	 *              initial slices.
	 */
	SliceEditor(const std::vector<boost::shared_ptr<Slice> >& initialSlices, unsigned int section, const util::box<int, 2>& region);

	/**
	 * Get a shared pointer to the current b/w slice image.
	 */
	boost::shared_ptr<IntensityImage> getSliceImage();

	/**
	 * Draw to the slice image.
	 */
	void draw(const util::point<double, 2>& position, double radius, bool foreground);

	/**
	 * Finish the slice editing.
	 *
	 * @return The performed changes as edits to the slices.
	 */
	SliceEdits finish();

private:

	void drawSlice(boost::shared_ptr<Slice> slice);

	// the slices to start with
	std::set<boost::shared_ptr<Slice> > _initialSlices;

	// section number for new slices
	unsigned int _section;

	// the edit region
	util::box<int, 2> _region;

	// b/w image of the current slices
	boost::shared_ptr<IntensityImage> _sliceImage;
};

#endif // SOPNET_SLICES_SLICE_EDITOR_H__

