#include "StackStore.h"

template <typename ImageType>
pipeline::Value<ImageStack<ImageType> >
StackStore<ImageType>::getImageStack(const util::box<unsigned int, 3>& box)
{
	pipeline::Value<ImageStack<ImageType> > stack = pipeline::Value<ImageStack<ImageType> >();

	for (unsigned int i = 0; i < box.depth(); ++i)
	{
		boost::shared_ptr<ImageType> image = getImage(box.project<2>(), box.min().z() + i);

		if (image->width()*image->height() == 0)
			UTIL_THROW_EXCEPTION(
					NoImageException,
					"no image found for box " << box << " at z " << (box.min().z() + i));

		stack->add(image);
	}

	stack->setResolution(_res);

	return stack;
}

template class StackStore<IntensityImage>;
