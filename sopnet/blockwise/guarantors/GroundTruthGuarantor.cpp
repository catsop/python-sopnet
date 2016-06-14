#include <pipeline/Process.h>
#include <pipeline/Value.h>

#include <core/evaluation/GroundTruthExtractor.h>

#include <blockwise/persistence/SegmentDescriptions.h>
#include <blockwise/persistence/exceptions.h>
#include <blockwise/blocks/Cores.h>

#include "GroundTruthGuarantor.h"

logger::LogChannel groundtruthguarantorlog("groundtruthguarantorlog", "[GroundTruthGuarantor] ");

GroundTruthGuarantor::GroundTruthGuarantor(
		const ProjectConfiguration&                projectConfiguration,
		boost::shared_ptr<SegmentStore>            segmentStore,
		boost::shared_ptr<SliceStore>              sliceStore,
		boost::shared_ptr<StackStore<LabelImage> > stackStore) :
	SliceGuarantor(projectConfiguration, sliceStore, 0),
	SegmentGuarantor(projectConfiguration, segmentStore, sliceStore, 0),
	SolutionGuarantor(projectConfiguration, segmentStore, sliceStore, 1, false, false, false),
	_segmentStore(segmentStore),
	_sliceStore(sliceStore),
	_stackStore(stackStore),
	_blockUtils(projectConfiguration) {

	LOG_DEBUG(groundtruthguarantorlog) << "core size is " << projectConfiguration.getCoreSize() << std::endl;
}

void
GroundTruthGuarantor::setComponentTreeExtractorParameters(
		const boost::shared_ptr<ComponentTreeExtractorParameters<LabelImage::value_type> > parameters) {

	_parameters = parameters;
}

Blocks
GroundTruthGuarantor::guaranteeGroundTruth(const Core& core) {

	LOG_DEBUG(groundtruthguarantorlog)
			<< "requesting ground truth for core ("
			<< core.x() << ", " << core.y() << ", " << core.z()
			<< ")" << std::endl;

	Blocks blocks = _blockUtils.getCoreBlocks(core);
	util::box<unsigned int, 3> bound = _blockUtils.getBoundingBox(blocks);

	pipeline::Value<ImageStack<LabelImage> > labelStack = _stackStore->getImageStack(bound);

	boost::shared_ptr<GroundTruthExtractor> extractor = boost::make_shared<GroundTruthExtractor>(false);

	extractor->setInput("ground truth sections", labelStack);

	pipeline::Value<Slices> combinedSlices;
	pipeline::Value<Segments> groundTruthSegments;
	combinedSlices   = extractor->getOutput("ground truth slices");
	groundTruthSegments = extractor->getOutput("ground truth segments");
	std::vector<boost::shared_ptr<Segment> > segments = groundTruthSegments->getSegments();

	SliceGuarantor::writeSlicesAndConflicts(
			*combinedSlices,
			ConflictSets(),
			blocks,
			blocks);

	// Segments
	std::map<unsigned int, unsigned int> featuresMap;
	boost::shared_ptr<Features> features = boost::make_shared<Features>();
	features->resize(groundTruthSegments->size(), 0);
	unsigned int i = 0;
	for (const boost::shared_ptr<Segment>& segment : segments) {
		featuresMap.emplace(segment->getId(), i++);
	}
	features->setSegmentIdsMap(featuresMap);

	SegmentGuarantor::writeSegmentsAndFeatures(
			*groundTruthSegments,
			*features,
			blocks);

	// Solution
	SegmentDescriptions segmentDescriptions;

	for (const boost::shared_ptr<Segment>& segment : segments) {

		SegmentDescription segmentDescription(*segment);

		segmentDescription.setFeatures(features->get(segment->getId()));

		segmentDescriptions.add(segmentDescription);
	}

	std::vector<SegmentHash> segmentHashes(segments.size());
	std::transform(
			std::begin(segmentDescriptions),
			std::end(segmentDescriptions),
			std::back_inserter(segmentHashes),
			[](const SegmentDescription& segment){
				return segment.getHash();
			});

	std::vector<std::set<SegmentHash> > assemblies = extractAssemblies(segmentHashes, segmentDescriptions);

	_segmentStore->storeSolution(assemblies, core);

	// there are no missing blocks
	return Blocks();
}
