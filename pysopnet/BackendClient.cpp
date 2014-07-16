#include <sopnet/block/LocalBlockManager.h>
#include <catmaid/persistence/django/DjangoSegmentStore.h>
#include <catmaid/persistence/django/CatmaidStackStore.h>
#include <catmaid/persistence/local/LocalStackStore.h>
#include <catmaid/persistence/local/LocalSliceStore.h>
#include <catmaid/persistence/local/LocalSegmentStore.h>
#include <catmaid/persistence/local/LocalSolutionStore.h>
#include "BackendClient.h"
#include "logging.h"

namespace python {

boost::shared_ptr<BlockManager>
BackendClient::createBlockManager(const ProjectConfiguration& configuration) {

	if (configuration.getBackendType() == ProjectConfiguration::Local) {

		LOG_USER(pylog) << "[BackendClient] create local block manager" << std::endl;

		boost::shared_ptr<LocalBlockManager> localBlockManager = boost::make_shared<LocalBlockManager>(
				LocalBlockManager(
						configuration.getVolumeSize(),
						configuration.getBlockSize(),
						configuration.getCoreSize()));

		return localBlockManager;
	}

	if (configuration.getBackendType() == ProjectConfiguration::Django) {

		LOG_USER(pylog) << "[BackendClient] create django block manager" << std::endl;

		_djangoBlockManager = DjangoBlockManager::getBlockManager(
				configuration.getCatmaidHost(),
				configuration.getCatmaidRawStackId(),
				configuration.getCatmaidProjectId());

		return _djangoBlockManager;
	}

	UTIL_THROW_EXCEPTION(UsageError, "unknown backend type " << configuration.getBackendType());
}

boost::shared_ptr<StackStore>
BackendClient::createStackStore(const ProjectConfiguration& configuration, StackType type) {

	if (configuration.getBackendType() == ProjectConfiguration::Local) {

		LOG_USER(pylog) << "[BackendClient] create local stack store for membranes" << std::endl;

		return boost::make_shared<LocalStackStore>(type == Raw ? "./raw" : "./membranes");
	}

	if (configuration.getBackendType() == ProjectConfiguration::Django) {

		LOG_USER(pylog) << "[BackendClient] create django stack store for membranes" << std::endl;

		if (type == Raw)
			return boost::make_shared<CatmaidStackStore>(
					configuration.getCatmaidHost(),
					configuration.getCatmaidProjectId(),
					configuration.getCatmaidRawStackId());
		else
			return boost::make_shared<CatmaidStackStore>(
					configuration.getCatmaidHost(),
					configuration.getCatmaidProjectId(),
					configuration.getCatmaidMembraneStackId());
	}

	UTIL_THROW_EXCEPTION(UsageError, "unknown backend type " << configuration.getBackendType());
}

boost::shared_ptr<SliceStore>
BackendClient::createSliceStore(const ProjectConfiguration& configuration) {

	if (configuration.getBackendType() == ProjectConfiguration::Local) {

		LOG_USER(pylog) << "[BackendClient] create local slice store" << std::endl;

		return boost::make_shared<LocalSliceStore>();
	}

	if (configuration.getBackendType() == ProjectConfiguration::Django) {

		LOG_USER(pylog) << "[BackendClient] create django slice store" << std::endl;

		if (!_djangoBlockManager)
			createBlockManager(configuration);

		_djangoSliceStore = boost::make_shared<DjangoSliceStore>(_djangoBlockManager);

		return _djangoSliceStore;
	}

	UTIL_THROW_EXCEPTION(UsageError, "unknown backend type " << configuration.getBackendType());
}

boost::shared_ptr<SegmentStore>
BackendClient::createSegmentStore(const ProjectConfiguration& configuration) {

	if (configuration.getBackendType() == ProjectConfiguration::Local) {

		LOG_USER(pylog) << "[BackendClient] create local segment store" << std::endl;

		return boost::make_shared<LocalSegmentStore>();
	}

	if (configuration.getBackendType() == ProjectConfiguration::Django) {

		LOG_USER(pylog) << "[BackendClient] create django segment store" << std::endl;

		if (!_djangoSliceStore)
			createSliceStore(configuration);

		return boost::make_shared<DjangoSegmentStore>(_djangoSliceStore);
	}

	UTIL_THROW_EXCEPTION(UsageError, "unknown backend type " << configuration.getBackendType());
}

boost::shared_ptr<SolutionStore>
BackendClient::createSolutionStore(const ProjectConfiguration& configuration) {

	if (configuration.getBackendType() == ProjectConfiguration::Local) {

		LOG_USER(pylog) << "[BackendClient] create local solution store" << std::endl;

		return boost::make_shared<LocalSolutionStore>();
	}

	if (configuration.getBackendType() == ProjectConfiguration::Django) {

		LOG_USER(pylog) << "[BackendClient] create django solution store" << std::endl;

		// there is no solution store for the django backend
		return boost::shared_ptr<SolutionStore>();
	}

	UTIL_THROW_EXCEPTION(UsageError, "unknown backend type " << configuration.getBackendType());
}

} // namespace python
