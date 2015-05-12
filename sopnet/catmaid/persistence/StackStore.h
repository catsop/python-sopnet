#ifndef SOPNET_CATMAIDSOPNET_PERSISTENCE_STACK_STORE_H__
#define SOPNET_CATMAIDSOPNET_PERSISTENCE_STACK_STORE_H__

#include <pipeline/Data.h>
#include <pipeline/Value.h>
#include <imageprocessing/ImageStack.h>
#include <util/box.hpp>
#include <util/exceptions.h>

struct NoImageException : virtual Exception {};

/**
 * Database abstraction for image stacks.
 */
class StackStore : public pipeline::Data
{
public:

	/**
	 * Return an ImageStack Value for the given Box by calling getImage for each 
	 * section (z-coordinate) contained in the Box.
	 */
	virtual pipeline::Value<ImageStack> getImageStack(const util::box<unsigned int, 3>& box);
	
protected:
	
	/**
	 * Return the image for the given section and rectangular bounding box, or an empty
	 * image if this is impossible (for instance, if the section number is invalid, or the
	 * bound does not overlap any image data in the section).
	 */
	virtual boost::shared_ptr<Image> getImage(
			const util::box<unsigned int, 2> bound,
			const unsigned int section) = 0;
};

#endif // SOPNET_CATMAIDSOPNET_PERSISTENCE_STACK_STORE_H__

