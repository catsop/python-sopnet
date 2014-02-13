#ifndef SOPNET_PYTHON_BACKEND_CLIENT_H__
#define SOPNET_PYTHON_BACKEND_CLIENT_H__

#include <pipeline/Value.h>
#include <sopnet/block/BlockManager.h>
#include <catmaid/persistence/StackStore.h>
#include <catmaid/persistence/SliceStore.h>
#include <catmaid/persistence/SegmentStore.h>
#include <catmaid/persistence/SolutionStore.h>
#include "ProjectConfiguration.h"

namespace python {

/**
 * Base class for backend clients. Provides helper functions to access the data 
 * stores for a given project configuration.
 */
class BackendClient {

protected:

	pipeline::Value<BlockManager> createBlockManager(const ProjectConfiguration& configuration);

	pipeline::Value<StackStore>   createStackStore(const ProjectConfiguration& configuration);

	pipeline::Value<SliceStore>   createSliceStore(const ProjectConfiguration& configuration);

	pipeline::Value<SegmentStore> createSegmentStore(const ProjectConfiguration& configuration);

	pipeline::Value<SolutionStore> createSolutionStore(const ProjectConfiguration& configuration);
};

} // namespace python

#endif // SOPNET_PYTHON_BACKEND_CLIENT_H__

