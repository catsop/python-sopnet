#ifndef SOPNET_PYTHON_SLICE_GUARANTOR_H__
#define SOPNET_PYTHON_SLICE_GUARANTOR_H__

#include <sopnet/block/Block.h>
#include "ProjectConfiguration.h"

namespace python {

class SliceGuarantor {

public:

	void fill(const Block& block, const ProjectConfiguration& configuration);
};

} // namespace python

#endif // SOPNET_PYTHON_SLICE_GUARANTOR_H__

