#include "SegmentGuarantor.h"
#include <util/Logger.h>
#include <sopnet/segments/SegmentExtractor.h>
#include <features/SegmentFeaturesExtractor.h>
#include <pipeline/Value.h>
#include <catmaid/EndExtractor.h>

logger::LogChannel segmentguarantorlog("segmentguarantorlog", "[SegmentGuarantor] ");

void
SegmentGuarantor::setSegmentStore(boost::shared_ptr<SegmentStore> segmentStore) {

	_segmentStore = segmentStore;
}

void
SegmentGuarantor::setSliceStore(boost::shared_ptr<SliceStore> sliceStore) {

	_sliceStore = sliceStore;
}

void
SegmentGuarantor::setRawStackStore(boost::shared_ptr<StackStore> rawStackStore) {

	_rawStackStore = rawStackStore;
}

Blocks SegmentGuarantor::guaranteeSegments(const Blocks& requestedBlocks) {

	if (!_segmentStore)
		UTIL_THROW_EXCEPTION(
				UsageError,
				"no segment store set");

	if (!_sliceStore)
		UTIL_THROW_EXCEPTION(
				UsageError,
				"no slice store set");

	if (!_rawStackStore)
		UTIL_THROW_EXCEPTION(
				UsageError,
				"no raw stack store set");

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

	// Now, check to see that we have all of the Slices we need.
	// If not, return a list of blocks that should have slices extracted before proceeding.
	if (!checkBlockSlices(sliceBlocks, needBlocks))
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

	slices = _sliceStore->retrieveSlices(sliceBlocks);

	LOG_DEBUG(segmentguarantorlog)
			<< "Read " << slices->size()
			<< " slices from blocks " << sliceBlocks
			<< ". Expanding blocks to fit." << std::endl;

	LOG_ALL(segmentguarantorlog) << "First found slices are:" << std::endl;
	foreach (boost::shared_ptr<Slice> slice, *slices)
		LOG_ALL(segmentguarantorlog) << "\t" << slice->getComponent()->getCenter() << ", " << slice->getSection() << std::endl;

	sliceBlocks.addAll(_blockManager->blocksInBox(slicesBoundingBox(*slices)));

	LOG_DEBUG(segmentguarantorlog) << "Expanded blocks are " << sliceBlocks << "." << std::endl;

	// Check again
	if (!checkBlockSlices(sliceBlocks, needBlocks))
	{
		return needBlocks;
	}

	// TODO: get slices only for new blocks
	slices = _sliceStore->retrieveSlices(sliceBlocks);

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

	// store the segments and features
	writeSegments(*segments, requestedBlocks);
	writeFeatures(*features);

	// mark the requested blocks as done
	foreach (boost::shared_ptr<Block> block, requestedBlocks)
	{
		block->setSegmentsFlag(true);
	}

	// needBlocks is empty
	return needBlocks;
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

bool
SegmentGuarantor::checkBlockSlices(
		const Blocks& sliceBlocks,
		Blocks&       needBlocks)
{
	bool ok = true;
	foreach (boost::shared_ptr<Block> block, sliceBlocks)
	{
		if (!block->getSlicesFlag())
		{
			ok = false;
			needBlocks.add(block);
		}
	}

	return ok;
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

void
SegmentGuarantor::writeSegments(const Segments& segments, const Blocks& blocks)
{
	foreach(boost::shared_ptr<Block> block, blocks)
	{
		Segments blockSegments;

		foreach (boost::shared_ptr<Segment> segment, segments.getSegments())
		{
			if (associated(*segment, *block))
			{
				blockSegments.add(segment);
			}
		}

		_segmentStore->associate(segments, *block);
	}
}

bool
SegmentGuarantor::associated(
		const Segment& segment,
		const Block& block)
{
	util::rect<unsigned int> blockRect = block;
	foreach (boost::shared_ptr<Slice> slice, segment.getSlices())
	{
		if (blockRect.intersects(
			static_cast<util::rect<unsigned int> >(slice->getComponent()->getBoundingBox())))
		{
			return true;
		}
	}
	
	return false;
}

void
SegmentGuarantor::writeFeatures(const Features& features) {

	LOG_DEBUG(segmentguarantorlog) << "storing features: " << features << std::endl;
	_segmentStore->storeFeatures(features);
}

