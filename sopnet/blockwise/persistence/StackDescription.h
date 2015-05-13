#ifndef SOPNET_BLOCKWISE_PERSISTENCE_STACK_DESCRIPTION_H__
#define SOPNET_BLOCKWISE_PERSISTENCE_STACK_DESCRIPTION_H__

#include "StackType.h"

/**
 * The type of image stack to request.
 */
struct StackDescription {

	std::string imageBase;

	std::string fileExtension;

	unsigned int tileSourceType, tileWidth, tileHeight;

	unsigned int width, height, depth;

	float resX, resY, resZ;

	unsigned int scale;

	unsigned int id, segmentationId;
};

#endif // SOPNET_BLOCKWISE_PERSISTENCE_STACK_DESCRIPTION_H__
