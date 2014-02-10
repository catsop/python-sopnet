#ifndef SOPNET_PYTHON_SLICE_GUARANTOR_H__
#define SOPNET_PYTHON_SLICE_GUARANTOR_H__

#include <pipeline/Value.h>
#include <sopnet/block/Block.h>
#include <sopnet/block/BlockManager.h>
#include <catmaidsopnet/persistence/StackStore.h>
#include <catmaidsopnet/persistence/SliceStore.h>
#include "SliceGuarantorParameters.h"
#include "ProjectConfiguration.h"

namespace python {

class SliceGuarantor {

public:

	/**
	 * Request the extraction and storage of slices in a block.
	 *
	 * @param blockLocation
	 *             The location of the requested block.
	 *
	 * @param parameters
	 *             Slice extraction parameters.
	 *
	 * @param configuration
	 *             Project specific configuration.
	 */
	void fill(
			const util::point3<unsigned int>& blockLocation,
			const SliceGuarantorParameters& parameters,
			const ProjectConfiguration& configuration);

private:

	pipeline::Value<BlockManager> createBlockManager(const ProjectConfiguration& configuration);
	pipeline::Value<StackStore>   createStackStore(const ProjectConfiguration& configuration);
	pipeline::Value<SliceStore>   createSliceStore(const ProjectConfiguration& configuration);
};

} // namespace python

#endif // SOPNET_PYTHON_SLICE_GUARANTOR_H__

