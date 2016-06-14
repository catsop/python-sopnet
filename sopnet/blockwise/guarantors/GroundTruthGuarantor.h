#ifndef GROUND_TRUTH_GUARANTOR_H__
#define GROUND_TRUTH_GUARANTOR_H__

#include <boost/shared_ptr.hpp>

#include <imageprocessing/ComponentTreeExtractorParameters.h>

#include <blockwise/ProjectConfiguration.h>
#include <blockwise/persistence/SegmentStore.h>
#include <blockwise/persistence/SliceStore.h>
#include <blockwise/persistence/StackStore.h>
#include <blockwise/blocks/BlockUtils.h>
#include <blockwise/blocks/Core.h>

#include <segments/SegmentHash.h>

#include "SliceGuarantor.h"
#include "SegmentGuarantor.h"
#include "SolutionGuarantor.h"

class GroundTruthGuarantor :
		private SliceGuarantor,
		private SegmentGuarantor,
		private SolutionGuarantor {

public:

	/**
	 * Create a new GroundTruthGuarantor using the given database stores.
	 *
	 * @param projectConfiguration
	 *              The ProjectConfiguration used to configure Block and Core
	 *              parameters.
	 *
	 * @param segmentStore
	 *              The SegmentStore to store segments extracted from ground
	 *              truth labels.
	 *
	 * @param sliceStore
	 *              The slice store to store slices extracted from ground
	 *              truth labels.
	 */
	GroundTruthGuarantor(
			const ProjectConfiguration&     projectConfiguration,
			boost::shared_ptr<SegmentStore> segmentStore,
			boost::shared_ptr<SliceStore>   sliceStore,
			boost::shared_ptr<StackStore<LabelImage> > stackStore);

	/**
	 * Set non-default parameters for the label region extraction.
	 */
	void setComponentTreeExtractorParameters(
			const boost::shared_ptr<ComponentTreeExtractorParameters<LabelImage::value_type> > parameters);

	/**
	 * Get the ground truth for the given core from a ground truth label
	 * stack.
	 *
	 * @param core
	 *              The core to extract the ground truth for.
	 */
	Blocks guaranteeGroundTruth(const Core& core);

private:

	boost::shared_ptr<SegmentStore> _segmentStore;
	boost::shared_ptr<SliceStore>   _sliceStore;
	
	boost::shared_ptr<ComponentTreeExtractorParameters<LabelImage::value_type> > _parameters;
	boost::shared_ptr<StackStore<LabelImage> >      _stackStore;

	BlockUtils _blockUtils;
};

#endif //GROUND_TRUTH_GUARANTOR_H__
