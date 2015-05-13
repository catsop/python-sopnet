#include "SegmentGuarantor.h"
#include <util/Logger.h>
#include <segments/SegmentExtractor.h>
#include <features/SegmentFeaturesExtractor.h>
#include <pipeline/Process.h>
#include <pipeline/Value.h>

logger::LogChannel segmentguarantorlog("segmentguarantorlog", "[SegmentGuarantor] ");

SegmentGuarantor::SegmentGuarantor(
		const ProjectConfiguration&     projectConfiguration,
		boost::shared_ptr<SegmentStore> segmentStore,
		boost::shared_ptr<SliceStore>   sliceStore,
		boost::shared_ptr<StackStore>   rawStackStore) :
	_segmentStore(segmentStore),
	_sliceStore(sliceStore),
	_rawStackStore(rawStackStore),
	_blockUtils(projectConfiguration) {}

Blocks
SegmentGuarantor::guaranteeSegments(const Blocks& requestedBlocks) {

	// do nothing, if the requested blocks have been extracted already
	if (alreadyPresent(requestedBlocks))
		return Blocks();

	// get all slices that can potentially be needed for segments in the 
	// requested blocks
	Blocks missingBlocks;
	boost::shared_ptr<Slices> slices = getSlices(requestedBlocks, missingBlocks);

	if (!missingBlocks.empty())
		return missingBlocks;

	LOG_DEBUG(segmentguarantorlog) << "Altogether, I have " << slices->size() << " slices" << std::endl;

	LOG_ALL(segmentguarantorlog) << "Finally found slices are:" << std::endl;
	for (boost::shared_ptr<Slice> slice : *slices)
		LOG_ALL(segmentguarantorlog) << "\t" << slice->getComponent()->getCenter() << ", " << slice->getSection() << std::endl;

	// the requested segments
	boost::shared_ptr<Segments> segments = boost::make_shared<Segments>();

	util::box<unsigned int, 3> requestBoundingBox = _blockUtils.getBoundingBox(requestedBlocks);

	// begin and end section of the request
	unsigned int zBegin = requestBoundingBox.min().z();
	unsigned int zEnd   = requestBoundingBox.max().z();

	boost::shared_ptr<Slices> nextSlices;

	// Special case: for the leftmost section in the stack (index 0) end
	// segments on the left side must be extracted.
	if (0 == zBegin) {
		nextSlices = boost::make_shared<Slices>();
	} else {
		nextSlices = collectSlicesByZ(*slices, zBegin);
		zBegin++;
	}

	for (unsigned int z = zBegin; z <= zEnd; ++z) {

		pipeline::Process<SegmentExtractor> extractor;

		// Collect slices for sections z - 1 and z
		boost::shared_ptr<Slices> prevSlices = nextSlices;
		nextSlices = collectSlicesByZ(*slices, z);

		// Set up the extractor
		extractor->setInput("previous slices", prevSlices);
		extractor->setInput("next slices", nextSlices);

		// and grab the segments.
		pipeline::Value<Segments> extractedSegments = extractor->getOutput("segments");

		LOG_DEBUG(segmentguarantorlog)
				<< "Got " << extractedSegments->size()
				<< " segments for ISI " << z << std::endl;

		segments->addAll(extractedSegments);
	}

	// sort out segments that do not overlap with the requested block
	segments = discardNonRequestedSegments(segments, requestedBlocks);

	// compute the features for all extracted segments
	boost::shared_ptr<Features> features = computeFeatures(segments);

	writeSegmentsAndFeatures(*segments, *features, requestedBlocks);

	// no slices were missing
	return Blocks();
}

bool
SegmentGuarantor::alreadyPresent(const Blocks& blocks) {

	for (const Block& block : blocks)
		if (!_segmentStore->getSegmentsFlag(block))
			return false;

	return true;
}

boost::shared_ptr<Slices>
SegmentGuarantor::getSlices(Blocks sliceBlocks, Blocks& missingBlocks) {

	// Blocks for which to extract slices. Expand sliceBlocks by z+1. We need to 
	// grab Slices from the first section of the next block
	_blockUtils.expand(
			sliceBlocks,
			0, 0, 1,
			0, 0, 0);

	LOG_DEBUG(segmentguarantorlog) << "asking for slices in the following blocks:" << sliceBlocks << std::endl;

	// Step one: retrieve all of the slices associated with the slice-request blocks.
	// Step two: expand the slice-request blocks to fit the boundaries of all of our new slices,
	//           then try again.

	// the slices needed to extract the requested segments
	boost::shared_ptr<Slices> slices = _sliceStore->getSlicesByBlocks(sliceBlocks, missingBlocks);

	// not all required slices are present, yet
	if (!missingBlocks.empty())
		return slices;

	LOG_DEBUG(segmentguarantorlog)
			<< "Read " << slices->size()
			<< " slices from blocks " << sliceBlocks
			<< ". Expanding blocks to fit." << std::endl;

	LOG_ALL(segmentguarantorlog) << "First found slices are:" << std::endl;
	for (boost::shared_ptr<Slice> slice : *slices)
		LOG_ALL(segmentguarantorlog) << "\t" << slice->getComponent()->getCenter() << ", " << slice->getSection() << std::endl;

	// expand the request blocks
	Blocks expandedSliceBlocks = _blockUtils.getBlocksInBox(slicesBoundingBox(*slices));

	LOG_DEBUG(segmentguarantorlog) << "Expanded blocks are " << expandedSliceBlocks << "." << std::endl;

	Blocks newSliceBlocks;
	for (const Block& block : expandedSliceBlocks)
		if (!sliceBlocks.contains(block))
			newSliceBlocks.add(block);

	boost::shared_ptr<Slices> newSlices = _sliceStore->getSlicesByBlocks(newSliceBlocks, missingBlocks);

	slices->addAll(*newSlices);

	// Load conflict sets for slices
	std::map<SliceHash, unsigned int> internalIdMap;

	for (const boost::shared_ptr<Slice> slice : *slices) {
		internalIdMap[slice->hashValue()] = slice->getId();
	}

	boost::shared_ptr<ConflictSets> conflictSets =
			_sliceStore->getConflictSetsByBlocks(expandedSliceBlocks, missingBlocks);

	for (const ConflictSet& conflictSet : *conflictSets) {
		std::vector<unsigned int> setInternalIds;
		setInternalIds.reserve(conflictSet.getSlices().size());

		for (const SliceHash& sliceHash : conflictSet.getSlices())
			if (internalIdMap.count(sliceHash))
				setInternalIds.push_back(internalIdMap[sliceHash]);

		slices->addConflicts(setInternalIds);
	}

	return slices;
}

boost::shared_ptr<Segments>
SegmentGuarantor::discardNonRequestedSegments(
		boost::shared_ptr<Segments> allSegments,
		const Blocks&               requestedBlocks) {

	boost::shared_ptr<Segments> requestedSegments = boost::make_shared<Segments>();

	for (boost::shared_ptr<Segment> segment : allSegments->getSegments()) {

		for (const Block& block : requestedBlocks) {

			if (overlaps(*segment, block)) {

				requestedSegments->add(segment);
				break;
			}
		}
	}

	LOG_DEBUG(segmentguarantorlog)
			<< "Discarded " << (allSegments->size() - requestedSegments->size())
			<< "/" << allSegments->size() << " segments." << std::endl;

	return requestedSegments;
}

void
SegmentGuarantor::writeSegmentsAndFeatures(
		const Segments& segments,
		const Features& features,
		const Blocks&   requestedBlocks) {

	for (const Block& block : requestedBlocks) {

		SegmentDescriptions blockSegments = getSegmentDescriptions(segments, features, block);
		_segmentStore->associateSegmentsToBlock(blockSegments, block);
	}
}

SegmentDescriptions
SegmentGuarantor::getSegmentDescriptions(
		const Segments& segments,
		const Features& features,
		const Block&    block) {

	SegmentDescriptions segmentDescriptions;

	for (boost::shared_ptr<Segment> segment : segments.getSegments()) {

		// only if the segment overlaps with the current block
		if (!overlaps(*segment, block))
			continue;

		// create a new segment description
		SegmentDescription segmentDescription(*segment);

		// add features
		segmentDescription.setFeatures(features.get(segment->getId()));

		// add to collection of segment descriptions for current block
		segmentDescriptions.add(segmentDescription);
	}

	return segmentDescriptions;
}

bool
SegmentGuarantor::overlaps(const Segment& segment, const Block& block) {

	unsigned int section = segment.getInterSectionInterval();

	util::box<unsigned int, 3> blockBoundingBox = _blockUtils.getBoundingBox(block);

	// test in z
	if (section < blockBoundingBox.min().z() ||
	    section > blockBoundingBox.max().z())
		return false;

	// test in x-y
	util::box<unsigned int, 2> blockRect = blockBoundingBox.project<2>();
	for (boost::shared_ptr<Slice> slice : segment.getSlices()) {

		util::box<unsigned int, 2> sliceBoundingBox = slice->getComponent()->getBoundingBox();

		if (blockRect.intersects(sliceBoundingBox))
			return true;
	}

	return false;
}

boost::shared_ptr<Slices>
SegmentGuarantor::collectSlicesByZ(
		Slices& slices,
		unsigned int z) const
{
	boost::shared_ptr<Slices> zSlices = boost::make_shared<Slices>();

	for (boost::shared_ptr<Slice> slice : slices)
	{
		if (slice->getSection() == z)
		{
			zSlices->add(slice);
			zSlices->setConflicts(slice->getId(), slices.getConflicts(slice->getId()));
		}
	}

	LOG_DEBUG(segmentguarantorlog) << "Collected " << zSlices->size() << " slices for z=" << z << std::endl;

	return zSlices;
}

util::box<unsigned int, 3>
SegmentGuarantor::slicesBoundingBox(const Slices& slices)
{
	if (slices.size() == 0)
		return util::box<unsigned int, 3>(0, 0, 0, 0, 0, 0);

	util::box<unsigned int, 2> bound(0, 0, 0, 0);
	unsigned int zMax = 0;
	unsigned int zMin = 0;

	for (const boost::shared_ptr<Slice> slice : slices)
	{
		if (bound.area() == 0) {

			bound = slice->getComponent()->getBoundingBox();
			zMax  = slice->getSection() + 1;
			zMin  = slice->getSection();

		} else {

			bound.fit(slice->getComponent()->getBoundingBox());
			zMax = std::max(zMax, slice->getSection() + 1);
			zMin = std::min(zMin, slice->getSection());
		}
	}

	return util::box<unsigned int, 3>(bound.min().x(), bound.min().y(), zMin, bound.max().x(), bound.max().y(), zMax);
}

boost::shared_ptr<Features>
SegmentGuarantor::computeFeatures(boost::shared_ptr<Segments> segments)
{
	util::box<unsigned int, 3> box = segments->boundingBox();
	Blocks blocks = _blockUtils.getBlocksInBox(box);
	util::box<unsigned int, 3> blocksBoundingBox = _blockUtils.getBoundingBox(blocks);
	blocksBoundingBox.max().z() =
			std::min(blocksBoundingBox.max().z(), _blockUtils.getVolumeBoundingBox().max().z());

	pipeline::Process<SegmentFeaturesExtractor> featuresExtractor;
	pipeline::Value<util::point<unsigned int, 3> > offset(blocksBoundingBox.min());
	pipeline::Value<Features> features;

	LOG_DEBUG(segmentguarantorlog)
			<< "Extracting features for " << segments->size()
			<< " segments" << std::endl;

	featuresExtractor->setInput("segments", segments);
	featuresExtractor->setInput("raw sections", _rawStackStore->getImageStack(blocksBoundingBox));
	featuresExtractor->setInput("crop offset", offset);

	features = featuresExtractor->getOutput("all features");

	LOG_DEBUG(segmentguarantorlog) << "Done extracting features" << std::endl;

	return features;
}

