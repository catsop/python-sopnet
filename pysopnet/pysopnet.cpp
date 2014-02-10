#include <boost/python.hpp>

#include <util/point3.hpp>
#include <sopnet/block/Block.h>
#include "ProjectConfiguration.h"
#include "SliceGuarantor.h"

namespace python {

/**
 * Defines all the python classes in the module libpysopnet. Here we decide 
 * which functions and data members we wish to expose.
 */
BOOST_PYTHON_MODULE(libpysopnet) {

	// point3<unsigned int>
	boost::python::class_<util::point3<unsigned int> >("point3", boost::python::init<unsigned int, unsigned int, unsigned int>())
			.def_readwrite("x", &util::point3<unsigned int>::x)
			.def_readwrite("y", &util::point3<unsigned int>::y)
			.def_readwrite("z", &util::point3<unsigned int>::z);

	// SliceGuarantorParameters
	boost::python::class_<SliceGuarantorParameters>("SliceGuarantorParameters")
			.def("setMaxSliceSize", &SliceGuarantorParameters::setMaxSliceSize)
			.def("getMaxSliceSize", &SliceGuarantorParameters::getMaxSliceSize);

	// ProjectConfiguration
	boost::python::class_<ProjectConfiguration>("ProjectConfiguration")
			.def("setDjangoUrl", &ProjectConfiguration::setDjangoUrl)
			.def("getDjangoUrl", &ProjectConfiguration::getDjangoUrl, boost::python::return_value_policy<boost::python::copy_const_reference>())
			.def("setBlockSize", &ProjectConfiguration::setBlockSize)
			.def("getBlockSize", &ProjectConfiguration::getBlockSize, boost::python::return_internal_reference<>());

	// Block
	boost::python::class_<Block>("Block")
			.def("getLocation", &Block::location);

	// SliceGuarantor
	boost::python::class_<SliceGuarantor>("SliceGuarantor")
			.def("fill", &SliceGuarantor::fill);
}

} // namespace python
