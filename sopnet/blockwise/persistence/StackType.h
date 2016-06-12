#ifndef SOPNET_BLOCKWISE_PERSISTENCE_STACK_TYPE_H__
#define SOPNET_BLOCKWISE_PERSISTENCE_STACK_TYPE_H__

/**
 * The type of image stack to request.
 */
enum StackType {

	Raw,
	Membrane,
	GroundTruth
};

static const char* const STACK_TYPE_NAME[] = {"raw", "membrane", "groundtruth"};

#endif // SOPNET_BLOCKWISE_PERSISTENCE_STACK_TYPE_H__

