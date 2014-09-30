#include "SegmentGuarantor.h"
#include <util/Logger.h>
#include <sopnet/segments/SegmentExtractor.h>
#include <features/SegmentFeaturesExtractor.h>
#include <pipeline/Process.h>
#include <pipeline/Value.h>

logger::LogChannel segmentguarantorlog("segmentguarantorlog", "[SegmentGuarantor] ");

SegmentGuarantor::SegmentGuarantor(
		boost::shared_ptr<SegmentStore> segmentStore,
		boost::shared_ptr<SliceStore>   sliceStore,
		boost::shared_ptr<StackStore>   rawStackStore) :
	_segmentStore(segmentStore),
	_sliceStore(sliceStore),
	_rawStackStore(rawStackStore) {}

Blocks
SegmentGuarantor::guaranteeSegments(const Blocks& requestedBlocks) {

	// remember the block manager to use
	_blockManager = requestedBlocks.getManager();

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
	foreach (boost::shared_ptr<Slice> slice, *slices)
		LOG_ALL(segmentguarantorlog) << "\t" << slice->getComponent()->getCenter() << ", " << slice->getSection() << std::endl;

	// the requested segments
	boost::shared_ptr<Segments> segments = boost::make_shared<Segments>();

	// begin and end section of the request
	unsigned int zBegin = requestedBlocks.location().z;
	unsigned int zEnd = zBegin + requestedBlocks.size().z;

	for (unsigned int z = zBegin; z < zEnd; ++z) {

		// If sections z and z + 1 exist in our whitelist
		if (_blockManager->isValidZ(z) && _blockManager->isValidZ(z + 1)) {

			pipeline::Process<SegmentExtractor> extractor;

			// Collect slices for sections z and z + 1
			boost::shared_ptr<Slices> prevSlices = collectSlicesByZ(*slices, z);
			boost::shared_ptr<Slices> nextSlices = collectSlicesByZ(*slices, z + 1);

			// Set up the extractor
			extractor->setInput("previous slices", prevSlices);
			extractor->setInput("next slices", nextSlices);

			// and grab the segments.
			pipeline::Value<Segments> extractedSegments = extractor->getOutput("segments");

			LOG_DEBUG(segmentguarantorlog)
					<< "Got " << extractedSegments->size()
					<< " segments" << std::endl;

			segments->addAll(extractedSegments);
		}
	}

	// compute the features for all extracted segments
	boost::shared_ptr<Features> features = computeFeatures(segments);

	writeSegmentsAndFeatures(*segments, *features, requestedBlocks);

	// no slices were missing
	return Blocks();
}

bool
SegmentGuarantor::alreadyPresent(const Blocks& blocks) {

	foreach (boost::shared_ptr<Block> block, blocks)
		if (!block->getSegmentsFlag())
			return false;

	return true;
}

boost::shared_ptr<Slices>
SegmentGuarantor::getSlices(Blocks sliceBlocks, Blocks& missingBlocks) {

	// Blocks for which to extract slices. Expand sliceBlocks by z+1. We need to 
	// grab Slices from the first section of the next block
	sliceBlocks.expand(util::point3<int>(0, 0, 1));

	LOG_DEBUG(segmentguarantorlog) << "asking for slices in the following blocks:" << std::endl;
	foreach (boost::shared_ptr<Block> block, sliceBlocks)
		LOG_DEBUG(segmentguarantorlog) << "\t" << block->getCoordinates() << std::endl;

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
	foreach (boost::shared_ptr<Slice> slice, *slices)
		LOG_ALL(segmentguarantorlog) << "\t" << slice->getComponent()->getCenter() << ", " << slice->getSection() << std::endl;

	// expand the request blocks
	Blocks expandedSliceBlocks = _blockManager->blocksInBox(slicesBoundingBox(*slices));

	LOG_DEBUG(segmentguarantorlog) << "Expanded blocks are " << sliceBlocks << "." << std::endl;

	Blocks newSliceBlocks;
	foreach (boost::shared_ptr<Block> block, expandedSliceBlocks)
		if (!sliceBlocks.contains(block))
			newSliceBlocks.add(block);

	boost::shared_ptr<Slices> newSlices = _sliceStore->getSlicesByBlocks(newSliceBlocks, missingBlocks);

	slices->addAll(*newSlices);

	return slices;
}

void
SegmentGuarantor::writeSegmentsAndFeatures(
		const Segments& segments,
		const Features& features,
		const Blocks&   requestedBlocks) {

	foreach (boost::shared_ptr<Block> block, requestedBlocks) {

		SegmentDescriptions blockSegments = getSegmentDescriptions(segments, features, *block);
		_segmentStore->associateSegmentsToBlock(blockSegments, *block);
	}
}

SegmentDescriptions
SegmentGuarantor::getSegmentDescriptions(
		const Segments& segments,
		const Features& features,
		const Block&    block) {

	SegmentDescriptions segmentDescriptions;

	foreach (boost::shared_ptr<Segment> segment, segments.getSegments()) {

		// only if the segment overlaps with the current block
		if (!overlaps(*segment, block))
			continue;

		// get the 2D bounding box of the segment
		util::rect<unsigned int> boundingBox(0, 0, 0, 0);
		foreach (boost::shared_ptr<Slice> slice, segment->getSlices()) {

			if (boundingBox.area() == 0)
				boundingBox = slice->getComponent()->getBoundingBox();
			else
				boundingBox.fit(slice->getComponent()->getBoundingBox());
		}

		// create a new segment description
		SegmentDescription segmentDescription(
				segment->getInterSectionInterval(),
				boundingBox);

		// add slice hashes
		if (segment->getDirection() == Left) {

			foreach (boost::shared_ptr<Slice> slice, segment->getTargetSlices())
				segmentDescription.addLeftSlice(slice->hashValue());
			foreach (boost::shared_ptr<Slice> slice, segment->getSourceSlices())
				segmentDescription.addRightSlice(slice->hashValue());

		} else {

			foreach (boost::shared_ptr<Slice> slice, segment->getTargetSlices())
				segmentDescription.addRightSlice(slice->hashValue());
			foreach (boost::shared_ptr<Slice> slice, segment->getSourceSlices())
				segmentDescription.addLeftSlice(slice->hashValue());
		}

		// add features
		segmentDescription.setFeatures(features.get(segment->getId()));

		// add to collection of segment descriptions for current block
		segmentDescriptions.add(segmentDescription);
	}

	return segmentDescriptions;
}

bool
SegmentGuarantor::overlaps(const Segment& segment, const Block& block) {

	unsigned int minSection = segment.getInterSectionInterval();
	unsigned int maxSection = segment.getInterSectionInterval() + 1;

	// test in z
	if (maxSection <  block.location().z ||
	    minSection >= block.location().z + block.size().z)
		return false;

	// test in x-y
	util::rect<unsigned int> blockRect = block;
	foreach (boost::shared_ptr<Slice> slice, segment.getSlices()) {

		util::rect<unsigned int> sliceBoundingBox = slice->getComponent()->getBoundingBox();

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

	foreach (boost::shared_ptr<Slice> slice, slices)
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

Box<>
SegmentGuarantor::slicesBoundingBox(const Slices& slices)
{
	if (slices.size() == 0)
		return Box<>();

	util::rect<unsigned int> bound(0, 0, 0, 0);
	unsigned int zMax = 0;
	unsigned int zMin = 0;

	foreach (const boost::shared_ptr<Slice> slice, slices)
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

	return Box<>(bound, zMin, zMax - zMin);
}

boost::shared_ptr<Features>
SegmentGuarantor::computeFeatures(boost::shared_ptr<Segments> segments)
{
	Box<> box = segments->boundingBox();
	Blocks blocks = _blockManager->blocksInBox(box);

	pipeline::Process<SegmentFeaturesExtractor> featuresExtractor;
	pipeline::Value<util::point3<unsigned int> > offset(blocks.location());
	pipeline::Value<Features> features;

	LOG_DEBUG(segmentguarantorlog)
			<< "Extracting features for " << segments->size()
			<< " segments" << std::endl;

	featuresExtractor->setInput("segments", segments);
	featuresExtractor->setInput("raw sections", _rawStackStore->getImageStack(blocks));
	featuresExtractor->setInput("crop offset", offset);

	features = featuresExtractor->getOutput("all features");

	LOG_DEBUG(segmentguarantorlog) << "Done extracting features" << std::endl;

	return features;
}

