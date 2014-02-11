#include "StackStore.h"

pipeline::Value<ImageStack>
StackStore::getImageStack(const Box<>& box)
{
	pipeline::Value<ImageStack> stack = pipeline::Value<ImageStack>();
	
	for (unsigned int i = 0; i < box.size().z; ++i)
	{
		stack->add(getImage(box, box.location().z + i));
	}
	
	return stack;
}
