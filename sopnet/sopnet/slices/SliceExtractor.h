#ifndef SOPNET_SLICE_EXTRACTOR_H__
#define SOPNET_SLICE_EXTRACTOR_H__

#include <boost/shared_ptr.hpp>

#include <pipeline/all.h>
#include <inference/LinearConstraints.h>
#include <imageprocessing/ComponentTreeExtractor.h>
#include "Slices.h"

// forward declaration
class ComponentTreeDownSampler;
class ComponentTreePruner;
class ComponentTreeConverter;

template <typename Precision>
class SliceExtractor : public pipeline::ProcessNode {

public:

	/**
	 * Create a new slice extractor for the given section.
	 *
	 * @param section
	 *              The section number that the extracted slices will have.
	 *
	 * @param downsample
	 *              Do not extract slices that are single children of their 
	 *              parents in the component tree.
	 *
	 * @param maxSliceMerges
	 *              Limit the height of the slice component tree, counting the 
	 *              height from the leafs. This will be overwritten by a program 
	 *              option of the same name, if set.
	 */
	SliceExtractor(
			unsigned int section,
			bool downsample,
			unsigned int maxSliceMerges = 3);

private:

	void onInputSet(const pipeline::InputSetBase& signal);

	// optional mser parameters to override the program options
	pipeline::Input<ComponentTreeExtractorParameters> _parameters;

	void extractSlices();

	boost::shared_ptr<ComponentTreeExtractor<Precision> > _componentExtractor;
	boost::shared_ptr<ComponentTreeExtractorParameters>   _defaultParameters;
	boost::shared_ptr<ComponentTreeDownSampler>           _downSampler;
	boost::shared_ptr<ComponentTreePruner>                _pruner;
	boost::shared_ptr<ComponentTreeConverter>             _converter;
};

#endif // SOPNET_SLICE_EXTRACTOR_H__

