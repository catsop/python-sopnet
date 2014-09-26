#include "StackStore.h"

pipeline::Value<ImageStack>
StackStore::getImageStack(const Box<>& box)
{
	pipeline::Value<ImageStack> stack = pipeline::Value<ImageStack>();
	
	for (unsigned int i = 0; i < box.size().z; ++i)
	{
		boost::shared_ptr<Image> image = getImage(box, box.location().z + i);

		if (image->width()*image->height() == 0)
			UTIL_THROW_EXCEPTION(
					NoImageException,
					"no image found for box " << box);

		stack->add(image);
	}
	
	return stack;
}
