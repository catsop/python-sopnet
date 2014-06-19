#include <sopnet/block/LocalBlockManager.h>
#include <catmaid/django/DjangoSegmentStore.h>
#include <catmaid/django/CatmaidStackStore.h>
#include <catmaid/persistence/LocalStackStore.h>
#include <catmaid/persistence/LocalSliceStore.h>
#include <catmaid/persistence/LocalSegmentStore.h>
#include <catmaid/persistence/LocalSolutionStore.h>
#include "BackendClient.h"
#include "logging.h"

namespace python {

boost::shared_ptr<BlockManager>
BackendClient::createBlockManager(const ProjectConfiguration& configuration) {

	LOG_USER(pylog) << "[BackendClient] create local block manager" << std::endl;

	if (configuration.getBackendType() == ProjectConfiguration::Local) {

		boost::shared_ptr<LocalBlockManager> localBlockManager = boost::make_shared<LocalBlockManager>(
				LocalBlockManager(
						configuration.getVolumeSize(),
						configuration.getBlockSize(),
						configuration.getCoreSize()));

		return localBlockManager;
	}

	if (configuration.getBackendType() == ProjectConfiguration::Django) {

		_djangoBlockManager = DjangoBlockManager::getBlockManager(
				configuration.getDjangoUrl(),
				configuration.getCatmaidStackId(),
				configuration.getCatmaidProjectId());

		return _djangoBlockManager;
	}

	UTIL_THROW_EXCEPTION(UsageError, "unknown backend type " << configuration.getBackendType());
}

boost::shared_ptr<StackStore>
BackendClient::createStackStore(const ProjectConfiguration& configuration, StackType type) {

	LOG_USER(pylog) << "[BackendClient] create local stack store for membranes" << std::endl;

	if (configuration.getBackendType() == ProjectConfiguration::Local) {

		return boost::make_shared<LocalStackStore>(type == Raw ? "./raw" : "./membranes");
	}

	if (configuration.getBackendType() == ProjectConfiguration::Django) {

		return boost::make_shared<CatmaidStackStore>(
				configuration.getCatmaidHost(),
				configuration.getCatmaidProjectId(),
				configuration.getCatmaidStackId());
	}

	UTIL_THROW_EXCEPTION(UsageError, "unknown backend type " << configuration.getBackendType());
}

boost::shared_ptr<SliceStore>
BackendClient::createSliceStore(const ProjectConfiguration& configuration) {

	LOG_USER(pylog) << "[BackendClient] create local slice store" << std::endl;

	if (configuration.getBackendType() == ProjectConfiguration::Local) {

		return boost::make_shared<LocalSliceStore>();
	}

	if (configuration.getBackendType() == ProjectConfiguration::Django) {

		if (!_djangoBlockManager)
			createBlockManager(configuration);

		_djangoSliceStore = boost::make_shared<DjangoSliceStore>(_djangoBlockManager);

		return _djangoSliceStore;
	}

	UTIL_THROW_EXCEPTION(UsageError, "unknown backend type " << configuration.getBackendType());
}

boost::shared_ptr<SegmentStore>
BackendClient::createSegmentStore(const ProjectConfiguration& configuration) {

	LOG_USER(pylog) << "[BackendClient] create local segment store" << std::endl;

	if (configuration.getBackendType() == ProjectConfiguration::Local) {

		return boost::make_shared<LocalSegmentStore>();
	}

	if (configuration.getBackendType() == ProjectConfiguration::Django) {

		if (!_djangoSliceStore)
			createSliceStore(configuration);

		return boost::make_shared<DjangoSegmentStore>(_djangoSliceStore);
	}

	UTIL_THROW_EXCEPTION(UsageError, "unknown backend type " << configuration.getBackendType());
}

boost::shared_ptr<SolutionStore>
BackendClient::createSolutionStore(const ProjectConfiguration& configuration) {

	LOG_USER(pylog) << "[BackendClient] create local solution store" << std::endl;

	if (configuration.getBackendType() == ProjectConfiguration::Local) {

		return boost::make_shared<LocalSolutionStore>();
	}

	if (configuration.getBackendType() == ProjectConfiguration::Django) {

		// there is no solution store for the django backend
		return boost::shared_ptr<SolutionStore>();
	}

	UTIL_THROW_EXCEPTION(UsageError, "unknown backend type " << configuration.getBackendType());
}

} // namespace python
