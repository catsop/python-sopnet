#include "SegmentGuarantor.h"
#include <util/Logger.h>
#include <sopnet/segments/SegmentExtractor.h>
#include <features/SegmentFeaturesExtractor.h>
#include <pipeline/Value.h>
#include <catmaid/EndExtractor.h>

logger::LogChannel segmentguarantorlog("segmentguarantorlog", "[SegmentGuarantor] ");

SegmentGuarantor::SegmentGuarantor(
		boost::shared_ptr<SegmentStore> segmentStore,
		boost::shared_ptr<SliceStore>   sliceStore,
		boost::shared_ptr<StackStore>   rawStackStore) :
	_segmentStore(segmentStore),
	_sliceStore(sliceStore),
	_rawStackStore(rawStackStore) {}

Blocks SegmentGuarantor::guaranteeSegments(const Blocks& requestedBlocks) {

	// remember the block manager to use
	_blockManager = requestedBlocks.getManager();

	// blocks with missing slices
	Blocks needBlocks;

	// blocks for which to extract slices
	Blocks sliceBlocks = requestedBlocks;

	// the slices needed to extract the requested segments
	boost::shared_ptr<Slices> slices;

	// the requested segments
	boost::shared_ptr<Segments> segments = boost::make_shared<Segments>();

	// begin and end section of the request
	unsigned int zBegin = requestedBlocks.location().z;
	unsigned int zEnd = zBegin + requestedBlocks.size().z;

	// are segments already extracted for the requested blocks?
	bool segmentsCached = true;

	// Expand sliceBlocks by z+1. We need to grab Slices from the first section of the next block
	sliceBlocks.expand(util::point3<int>(0, 0, 1));

	LOG_DEBUG(segmentguarantorlog) << "asking for slices in the following blocks:" << std::endl;
	foreach (boost::shared_ptr<Block> block, sliceBlocks)
		LOG_DEBUG(segmentguarantorlog) << "\t" << block->getCoordinates() << std::endl;

	// Check to see if we have any blocks for which the extraction is necessary.
	foreach (boost::shared_ptr<Block> block, requestedBlocks)
	{
		segmentsCached &= block->getSegmentsFlag();
	}

	// If not, return empty blocks.
	if (segmentsCached)
	{
		return needBlocks;
	}

	// OK, here we go.

	// Step one: retrieve all of the slices associated with the slice-request blocks.
	// Step two: expand the slice-request blocks to fit the boundaries of all of our new slices,
	//           then try again.
	//
	// The second step is taken because we could have the situation in which a slice overlaps with
	// another across sections that does not belong to a conflict-clique that exists within the
	// requested geometry.

	Blocks missingBlocks;
	slices = _sliceStore->getSlicesByBlocks(sliceBlocks, missingBlocks);

	if (!missingBlocks.empty())
		return missingBlocks;

	LOG_DEBUG(segmentguarantorlog)
			<< "Read " << slices->size()
			<< " slices from blocks " << sliceBlocks
			<< ". Expanding blocks to fit." << std::endl;

	LOG_ALL(segmentguarantorlog) << "First found slices are:" << std::endl;
	foreach (boost::shared_ptr<Slice> slice, *slices)
		LOG_ALL(segmentguarantorlog) << "\t" << slice->getComponent()->getCenter() << ", " << slice->getSection() << std::endl;

	sliceBlocks.addAll(_blockManager->blocksInBox(slicesBoundingBox(*slices)));

	LOG_DEBUG(segmentguarantorlog) << "Expanded blocks are " << sliceBlocks << "." << std::endl;

	// TODO: get slices only for new blocks
	slices = _sliceStore->getSlicesByBlocks(sliceBlocks, missingBlocks);

	if (!missingBlocks.empty())
		return missingBlocks;

	LOG_DEBUG(segmentguarantorlog) << "Altogether, I have " << slices->size() << " slices" << std::endl;

	LOG_ALL(segmentguarantorlog) << "Finally found slices are:" << std::endl;
	foreach (boost::shared_ptr<Slice> slice, *slices)
		LOG_ALL(segmentguarantorlog) << "\t" << slice->getComponent()->getCenter() << ", " << slice->getSection() << std::endl;

	for (unsigned int z = zBegin; z < zEnd; ++z)
	{
		// If sections z and z + 1 exist in our whitelist
		if (_blockManager->isValidZ(z) && _blockManager->isValidZ(z + 1))
		{
			pipeline::Value<Segments> extractedSegments;

			boost::shared_ptr<SegmentExtractor> extractor = boost::make_shared<SegmentExtractor>();

			// Collect slices for sections z and z + 1
			boost::shared_ptr<Slices> prevSlices = collectSlicesByZ(*slices, z);
			boost::shared_ptr<Slices> nextSlices = collectSlicesByZ(*slices, z + 1);

			// Set up the extractor
			extractor->setInput("previous slices", prevSlices);
			extractor->setInput("next slices", nextSlices);

			// and grab the segments.
			extractedSegments = extractor->getOutput("segments");

			LOG_DEBUG(segmentguarantorlog)
					<< "Got " << extractedSegments->size()
					<< " segments" << std::endl;

			segments->addAll(extractedSegments);
		}
	}

	// compute the features for all extracted segments
	boost::shared_ptr<Features> features = guaranteeFeatures(segments);

	writeSegmentsAndFeatures(*segments, *features, requestedBlocks);

	// mark the requested blocks as done
	foreach (boost::shared_ptr<Block> block, requestedBlocks)
	{
		block->setSegmentsFlag(true);
	}

	// needBlocks is empty
	return needBlocks;
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

	util::rect<unsigned int> bound = slices[0]->getComponent()->getBoundingBox();
	unsigned int zMax = slices[0]->getSection() + 1;
	unsigned int zMin = slices[0]->getSection();

	foreach (const boost::shared_ptr<Slice> slice, slices)
	{
		bound.fit(slice->getComponent()->getBoundingBox());

		if (zMax < slice->getSection() + 1)
		{
			zMax = slice->getSection() + 1;
		}

		if (zMin > slice->getSection())
		{
			zMin = slice->getSection();
		}
	}

	return Box<>(bound, zMin, zMax - zMin);
}

boost::shared_ptr<Features>
SegmentGuarantor::guaranteeFeatures(boost::shared_ptr<Segments> segments)
{
	Box<> box = segments->boundingBox();
	boost::shared_ptr<Blocks> blocks = _blockManager->blocksInBox(box);
	pipeline::Value<util::point3<unsigned int> > offset(blocks->location());
	boost::shared_ptr<SegmentFeaturesExtractor> featuresExtractor =
		boost::make_shared<SegmentFeaturesExtractor>();
	pipeline::Value<Features> features;

	LOG_DEBUG(segmentguarantorlog) << "Extracting features for " << segments->size() <<
		" segments" << std::endl;

	featuresExtractor->setInput("segments", segments);
	featuresExtractor->setInput("raw sections", _rawStackStore->getImageStack(*blocks));
	featuresExtractor->setInput("crop offset", offset);

	features = featuresExtractor->getOutput("all features");

	LOG_DEBUG(segmentguarantorlog) << "Done extracting features" << std::endl;

	return features;
}

