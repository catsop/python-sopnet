#ifndef SOPNET_PYTHON_BACKEND_CLIENT_H__
#define SOPNET_PYTHON_BACKEND_CLIENT_H__

#include <catmaid/ProjectConfiguration.h>
#include <catmaid/persistence/StackStore.h>
#include <catmaid/persistence/StackType.h>
#include <catmaid/persistence/SliceStore.h>
#include <catmaid/persistence/SegmentStore.h>

namespace python {

/**
 * Base class for backend clients. Provides helper functions to access the data 
 * stores for a given project configuration.
 */
class BackendClient {

protected:

	boost::shared_ptr<StackStore>   createStackStore(const ProjectConfiguration& configuration, StackType type);

	boost::shared_ptr<SliceStore>   createSliceStore(const ProjectConfiguration& configuration);

	boost::shared_ptr<SegmentStore> createSegmentStore(const ProjectConfiguration& configuration);
};

} // namespace python

#endif // SOPNET_PYTHON_BACKEND_CLIENT_H__

