#include "StackStore.h"

pipeline::Value<ImageStack>
StackStore::getImageStack(const util::box<unsigned int, 3>& box)
{
	pipeline::Value<ImageStack> stack = pipeline::Value<ImageStack>();

	for (unsigned int i = 0; i < box.depth(); ++i)
	{
		boost::shared_ptr<Image> image = getImage(box.project<2>(), box.min().z() + i);

		if (image->width()*image->height() == 0)
			UTIL_THROW_EXCEPTION(
					NoImageException,
					"no image found for box " << box << " at z " << (box.min().z() + i));

		stack->add(image);
	}

	stack->setResolution(_res);

	return stack;
}
