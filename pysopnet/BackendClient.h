#ifndef SOPNET_PYTHON_BACKEND_CLIENT_H__
#define SOPNET_PYTHON_BACKEND_CLIENT_H__

#include <catmaid/persistence/BlockManager.h>
#include <catmaid/persistence/django/DjangoBlockManager.h>
#include <catmaid/persistence/django/DjangoSliceStore.h>
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

	/**
	 * The type of image stack to request.
	 */
	enum StackType {

		Raw,
		Membrane
	};

	boost::shared_ptr<BlockManager> createBlockManager(const ProjectConfiguration& configuration);

	boost::shared_ptr<StackStore>   createStackStore(const ProjectConfiguration& configuration, StackType type);

	boost::shared_ptr<SliceStore>   createSliceStore(const ProjectConfiguration& configuration);

	boost::shared_ptr<SegmentStore> createSegmentStore(const ProjectConfiguration& configuration);

// 	boost::shared_ptr<SolutionStore> createSolutionStore(const ProjectConfiguration& configuration);

private:

	boost::shared_ptr<DjangoBlockManager> _djangoBlockManager;

	boost::shared_ptr<DjangoSliceStore>   _djangoSliceStore;
};

} // namespace python

#endif // SOPNET_PYTHON_BACKEND_CLIENT_H__

