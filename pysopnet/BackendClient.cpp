#include <sopnet/block/LocalBlockManager.h>
#include <catmaid/persistence/LocalStackStore.h>
#include <catmaid/persistence/LocalSliceStore.h>
#include <catmaid/persistence/LocalSegmentStore.h>
#include <catmaid/persistence/LocalSolutionStore.h>
#include "BackendClient.h"
#include "logging.h"

namespace python {

pipeline::Value<BlockManager>
BackendClient::createBlockManager(const ProjectConfiguration& configuration) {

	LOG_USER(pylog) << "[BackendClient] create local block manager" << std::endl;

	// TODO: create one based on provided configuration
	pipeline::Value<LocalBlockManager> localBlockManager(
			LocalBlockManager(
					configuration.getVolumeSize(),
					configuration.getBlockSize(),
					configuration.getCoreSize()));

	return localBlockManager;
}

pipeline::Value<StackStore>
BackendClient::createStackStore(const ProjectConfiguration& /*configuration*/, StackType type) {

	LOG_USER(pylog) << "[BackendClient] create local stack store for membranes" << std::endl;

	// TODO: create one based on provided configuration
	pipeline::Value<LocalStackStore> localStackStore(LocalStackStore(type == Raw ? "./raw" : "./membranes"));

	return localStackStore;
}

pipeline::Value<SliceStore>
BackendClient::createSliceStore(const ProjectConfiguration& /*configuration*/) {

	LOG_USER(pylog) << "[BackendClient] create local slice store" << std::endl;

	// TODO: create one based on provided configuration
	pipeline::Value<LocalSliceStore> localSliceStore;

	return localSliceStore;
}

pipeline::Value<SegmentStore>
BackendClient::createSegmentStore(const ProjectConfiguration& /*configuration*/) {

	LOG_USER(pylog) << "[BackendClient] create local segment store" << std::endl;

	// TODO: create one based on provided configuration
	pipeline::Value<LocalSegmentStore> localSegmentStore;

	return localSegmentStore;
}

pipeline::Value<SolutionStore>
BackendClient::createSolutionStore(const ProjectConfiguration& /*configuration*/) {

	LOG_USER(pylog) << "[BackendClient] create local solution store" << std::endl;

	// TODO: create one based on provided configuration
	pipeline::Value<LocalSolutionStore> localSolutionStore;

	return localSolutionStore;
}

} // namespace python
