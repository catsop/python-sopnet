#include <boost/python.hpp>
#include <boost/python/exception_translator.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

#include <util/exceptions.h>
#include <util/point.hpp>
#include <catmaid/ProjectConfiguration.h>
#include <catmaid/blocks/Block.h>
#include <catmaid/persistence/StackDescription.h>
#include <catmaid/persistence/StackType.h>
#include <sopnet/segments/SegmentHash.h>
#include "SliceGuarantor.h"
#include "SegmentGuarantor.h"
#include "SolutionGuarantor.h"
#include "Locations.h"
#include "logging.h"

#ifdef HAVE_GUROBI
#include <gurobi_c++.h>
#endif

namespace python {

/**
 * Translates an Exception into a python exception.
 */
void translateException(const Exception& e) {

	if (boost::get_error_info<error_message>(e))
		PyErr_SetString(PyExc_RuntimeError, boost::get_error_info<error_message>(e)->c_str());
	else
		PyErr_SetString(PyExc_RuntimeError, e.what());
}

#ifdef HAVE_GUROBI
/**
 * Translates a Gurobi exception into a python exception.
 */
void translateGRBException(const GRBException& e) {

	PyErr_SetString(PyExc_RuntimeError, e.getMessage().c_str());
}
#endif

/**
 * Defines all the python classes in the module libpysopnet. Here we decide 
 * which functions and data members we wish to expose.
 */
BOOST_PYTHON_MODULE(libpysopnet) {

	boost::python::register_exception_translator<Exception>(&translateException);

#ifdef HAVE_GUROBI
	boost::python::register_exception_translator<GRBException>(&translateGRBException);
#endif

	// setLogLevel
	boost::python::def("setLogLevel", setLogLevel);

	// Segment hash_value (by slice hashes)
	boost::python::class_<std::vector<SliceHash> >("SliceHashVector")
			.def(boost::python::vector_indexing_suite<std::vector<SliceHash> >());
	boost::python::def(
			"segmentHashValue",
			static_cast<SegmentHash (*)(const std::vector<SliceHash>&, const std::vector<SliceHash>&)>(hash_value));

	// point<unsigned int, 3>
	boost::python::class_<util::point<unsigned int, 3> >("point3", boost::python::init<unsigned int, unsigned int, unsigned int>())
			.add_property("x", &util::point<unsigned int, 3>::__get_x, &util::point<unsigned int, 3>::__set_x)
			.add_property("y", &util::point<unsigned int, 3>::__get_y, &util::point<unsigned int, 3>::__set_y)
			.add_property("z", &util::point<unsigned int, 3>::__get_z, &util::point<unsigned int, 3>::__set_z);

	// SliceGuarantorParameters
	boost::python::class_<SliceGuarantorParameters>("SliceGuarantorParameters")
			.def("setMaxSliceSize", &SliceGuarantorParameters::setMaxSliceSize)
			.def("getMaxSliceSize", &SliceGuarantorParameters::getMaxSliceSize)
			.def("setMinSliceSize", &SliceGuarantorParameters::setMinSliceSize)
			.def("getMinSliceSize", &SliceGuarantorParameters::getMinSliceSize)
			.def("membraneIsBright", &SliceGuarantorParameters::membraneIsBright)
			.def("setMembraneIsBright", &SliceGuarantorParameters::setMembraneIsBright);

	// SegmentGuarantorParameters
	boost::python::class_<SegmentGuarantorParameters>("SegmentGuarantorParameters");

	// SolutionGuarantorParameters
	boost::python::class_<SolutionGuarantorParameters>("SolutionGuarantorParameters")
			.def("setCorePadding", &SolutionGuarantorParameters::setCorePadding)
			.def("setForceExplanation", &SolutionGuarantorParameters::setForceExplanation)
			.def("setReadCosts", &SolutionGuarantorParameters::setReadCosts)
			.def("setStoreCosts", &SolutionGuarantorParameters::setStoreCosts);

	// ProjectConfiguration
	boost::python::class_<ProjectConfiguration>("ProjectConfiguration")
			.def("setBackendType", &ProjectConfiguration::setBackendType)
			.def("getBackendType", &ProjectConfiguration::getBackendType)
			.def("setCatmaidStack", &ProjectConfiguration::setCatmaidStack)
			.def("getCatmaidStack", &ProjectConfiguration::getCatmaidStack, boost::python::return_value_policy<boost::python::copy_const_reference>())
			.def("setBlockSize", &ProjectConfiguration::setBlockSize)
			.def("getBlockSize", &ProjectConfiguration::getBlockSize, boost::python::return_internal_reference<>())
			.def("setCoreSize", &ProjectConfiguration::setCoreSize)
			.def("getCoreSize", &ProjectConfiguration::getCoreSize, boost::python::return_internal_reference<>())
			.def("setVolumeSize", &ProjectConfiguration::setVolumeSize)
			.def("getVolumeSize", &ProjectConfiguration::getVolumeSize, boost::python::return_internal_reference<>())
			.def("setComponentDirectory", &ProjectConfiguration::setComponentDirectory)
			.def("getComponentDirectory", &ProjectConfiguration::getComponentDirectory, boost::python::return_value_policy<boost::python::copy_const_reference>())
			.def("setLocalFeatureWeights", &ProjectConfiguration::setLocalFeatureWeights)
			.def("getLocalFeatureWeights", &ProjectConfiguration::getLocalFeatureWeights, boost::python::return_value_policy<boost::python::copy_const_reference>())
			.def("setPostgreSqlHost", &ProjectConfiguration::setPostgreSqlHost)
			.def("getPostgreSqlHost", &ProjectConfiguration::getPostgreSqlHost, boost::python::return_value_policy<boost::python::copy_const_reference>())
			.def("setPostgreSqlPort", &ProjectConfiguration::setPostgreSqlPort)
			.def("getPostgreSqlPort", &ProjectConfiguration::getPostgreSqlPort, boost::python::return_value_policy<boost::python::copy_const_reference>())
			.def("setPostgreSqlUser", &ProjectConfiguration::setPostgreSqlUser)
			.def("getPostgreSqlUser", &ProjectConfiguration::getPostgreSqlUser, boost::python::return_value_policy<boost::python::copy_const_reference>())
			.def("setPostgreSqlPassword", &ProjectConfiguration::setPostgreSqlPassword)
			.def("getPostgreSqlPassword", &ProjectConfiguration::getPostgreSqlPassword, boost::python::return_value_policy<boost::python::copy_const_reference>())
			.def("setPostgreSqlDatabase", &ProjectConfiguration::setPostgreSqlDatabase)
			.def("getPostgreSqlDatabase", &ProjectConfiguration::getPostgreSqlDatabase, boost::python::return_value_policy<boost::python::copy_const_reference>());

	// BackendType
	boost::python::enum_<ProjectConfiguration::BackendType>("BackendType")
			.value("Local", ProjectConfiguration::Local)
			.value("Django", ProjectConfiguration::Django)
			.value("PostgreSql", ProjectConfiguration::PostgreSql);

	// StackType
	boost::python::enum_<StackType>("StackType")
			.value("Raw", Raw)
			.value("Membrane", Membrane);

	// StackDescription
	boost::python::class_<StackDescription>("StackDescription")
			.def_readwrite("imageBase", &StackDescription::imageBase)
			.def_readwrite("fileExtension", &StackDescription::fileExtension)
			.def_readwrite("tileSourceType", &StackDescription::tileSourceType)
			.def_readwrite("tileWidth", &StackDescription::tileWidth)
			.def_readwrite("tileHeight", &StackDescription::tileHeight)
			.def_readwrite("width", &StackDescription::width)
			.def_readwrite("height", &StackDescription::height)
			.def_readwrite("depth", &StackDescription::depth)
			.def_readwrite("scale", &StackDescription::scale)
			.def_readwrite("id", &StackDescription::id)
			.def_readwrite("segmentationId", &StackDescription::segmentationId);

	// Locations
	boost::python::class_<Locations>("Locations")
			.def(boost::python::vector_indexing_suite<Locations>());

	// SliceGuarantor
	boost::python::class_<SliceGuarantor>("SliceGuarantor")
			.def("fill", &SliceGuarantor::fill);

	// SegmentGuarantor
	boost::python::class_<SegmentGuarantor>("SegmentGuarantor")
			.def("fill", &SegmentGuarantor::fill);

	// SolutionGuarantor
	boost::python::class_<SolutionGuarantor>("SolutionGuarantor")
			.def("fill", &SolutionGuarantor::fill);
}

} // namespace python
