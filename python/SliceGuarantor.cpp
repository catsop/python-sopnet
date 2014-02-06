#include "SliceGuarantor.h"
#include "logging.h"

namespace python {

void
SliceGuarantor::fill(const Block& block, const ProjectConfiguration& configuration) {

	LOG_USER(pylog) << "[SliceGuarantor] fill called for block " << block.location() << std::endl;
}

} // namespace python
