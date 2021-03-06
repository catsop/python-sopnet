#ifndef SOPNET_BLOCKWISE_PERSISTENCE_BACKEND_CLIENT_H__
#define SOPNET_BLOCKWISE_PERSISTENCE_BACKEND_CLIENT_H__

#include <blockwise/ProjectConfiguration.h>
#include <blockwise/persistence/StackStore.h>
#include <blockwise/persistence/StackType.h>
#include <blockwise/persistence/SliceStore.h>
#include <blockwise/persistence/SegmentStore.h>

/**
 * Base class for backend clients. Provides helper functions to access the data 
 * stores for a given project configuration.
 */
class BackendClient {

public:

	/**
	 * Fill in the details of an imcomplete project configuration. You have to 
	 * provide a critical mass, depending on the chosen backend type (of which 
	 * currently on PosgreSql is implemented):
	 *
	 *   PosgreSql:
	 *
	 *    host
	 *    port
	 *    user
	 *    password
	 *    database
	 *    segmentationConfigurationId
	 *    componentDirectory
	 */
	void fillProjectConfiguration(ProjectConfiguration& configuration);

	template <typename ImageType>
	boost::shared_ptr<StackStore<ImageType> > createStackStore(const ProjectConfiguration& configuration, const StackType type);

	boost::shared_ptr<SliceStore>   createSliceStore(const ProjectConfiguration& configuration, const StackType type);

	boost::shared_ptr<SegmentStore> createSegmentStore(const ProjectConfiguration& configuration, const StackType type);
};

#endif // SOPNET_BLOCKWISE_PERSISTENCE_BACKEND_CLIENT_H__

