#include <util/ProgramOptions.h>
#include <imageprocessing/ComponentTree.h>
#include <imageprocessing/ComponentTreeDownSampler.h>
#include <imageprocessing/ComponentTreePruner.h>
#include <pipeline/Value.h>
#include "ComponentTreeConverter.h"
#include "SliceExtractor.h"

static logger::LogChannel sliceextractorlog("sliceextractorlog", "[SliceExtractor] ");

util::ProgramOption optionInvertSliceMaps(
		util::_module           = "sopnet",
		util::_long_name        = "invertSliceMaps",
		util::_description_text = "Invert the meaning of the slice map. The default "
		                          "(not inverting) is: bright area = neuron hypotheses.");

util::ProgramOption optionMinSliceSize(
		util::_module           = "sopnet",
		util::_long_name        = "minSliceSize",
		util::_description_text = "The minimal size of a neuron slice in pixels.",
		util::_default_value    = 10);

util::ProgramOption optionMaxSliceSize(
		util::_module           = "sopnet",
		util::_long_name        = "maxSliceSize",
		util::_description_text = "The maximal size of a neuron slice in pixels.",
		util::_default_value    = 100*100);

template <typename Precision, typename ImageType>
SliceExtractor<Precision, ImageType>::SliceExtractor(
		unsigned int section,
		bool downsample,
		unsigned int maxSliceMerges) :
	_componentExtractor(boost::make_shared<ComponentTreeExtractor<Precision, ImageType> >()),
	_defaultParameters(boost::make_shared<ComponentTreeExtractorParameters<typename ImageType::value_type> >()),
	_downSampler(boost::make_shared<ComponentTreeDownSampler>()),
	_pruner(boost::make_shared<ComponentTreePruner>()),
	_converter(boost::make_shared<ComponentTreeConverter>(section)) {

	registerInput(_componentExtractor->getInput("image"), "membrane");
	registerInput(_parameters, "parameters");
	registerOutput(_converter->getOutput("slices"), "slices");
	registerOutput(_converter->getOutput("conflict sets"), "conflict sets");

	_parameters.registerCallback(&SliceExtractor<Precision, ImageType>::onInputSet, this);

	// set default parameters from program options
	_defaultParameters->darkToBright      =  optionInvertSliceMaps;
	_defaultParameters->minSize           =  optionMinSliceSize;
	_defaultParameters->maxSize           =  optionMaxSliceSize;

	LOG_DEBUG(sliceextractorlog)
			<< "extracting slices with min size " << optionMinSliceSize.as<int>()
			<< ", max size " << optionMaxSliceSize.as<int>()
			<< ", and max tree depth " << maxSliceMerges
			<< std::endl;

	// setup internal pipeline
	_componentExtractor->setInput("parameters", _defaultParameters);

	if (downsample) {

		_downSampler->setInput(_componentExtractor->getOutput());
		_pruner->setInput("component tree", _downSampler->getOutput());

	} else {

		_pruner->setInput("component tree", _componentExtractor->getOutput());
	}
	_pruner->setInput("max height", pipeline::Value<int>(maxSliceMerges));
	_converter->setInput(_pruner->getOutput());
}

template <typename Precision, typename ImageType>
void
SliceExtractor<Precision, ImageType>::onInputSet(const pipeline::InputSetBase&) {

	LOG_ALL(sliceextractorlog) << "using non-default parameters" << std::endl;

	// don't use the default
	_componentExtractor->setInput("parameters", _parameters);
}

// explicit template instantiations
template class SliceExtractor<unsigned char, IntensityImage>;
template class SliceExtractor<unsigned short, IntensityImage>;
