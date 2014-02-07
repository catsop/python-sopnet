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
	 */
	void fill(const Block& block, const SliceGuarantorParameters& paramters, const ProjectConfiguration& configuration);

private:

	pipeline::Value<BlockManager> createBlockManager(const ProjectConfiguration& configuration);
	pipeline::Value<StackStore>   createStackStore(const ProjectConfiguration& configuration);
	pipeline::Value<SliceStore>   createSliceStore(const ProjectConfiguration& configuration);
};

} // namespace python

#endif // SOPNET_PYTHON_SLICE_GUARANTOR_H__

