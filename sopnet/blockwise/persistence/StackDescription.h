#ifndef SOPNET_CATMAID_PERSISTENCE_STACK_DESCRIPTION_H__
#define SOPNET_CATMAID_PERSISTENCE_STACK_DESCRIPTION_H__

#include "StackType.h"

/**
 * The type of image stack to request.
 */
struct StackDescription {

	std::string imageBase;

	std::string fileExtension;

	unsigned int tileSourceType, tileWidth, tileHeight;

	unsigned int width, height, depth;

	unsigned int scale;

	unsigned int id, segmentationId;
};

#endif // SOPNET_CATMAID_PERSISTENCE_STACK_DESCRIPTION_H__
