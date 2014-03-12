#include "SegmentGuarantor.h"
#include <util/Logger.h>
#include <sopnet/segments/SegmentExtractor.h>
#include <features/SegmentFeaturesExtractor.h>
#include <pipeline/Value.h>

logger::LogChannel segmentguarantorlog("segmentguarantorlog", "[SegmentGuarantor] ");

SegmentGuarantor::SegmentGuarantor()
{
	registerInput(_blocks, "blocks");
	registerInput(_segmentStore, "segment store");
	registerInput(_sliceStore, "slice store");
	registerInput(_rawImageStore, "stack store", pipeline::Optional);
	
	registerOutput(_needBlocks, "slice blocks");
}


pipeline::Value<Blocks> SegmentGuarantor::guaranteeSegments()
{
	updateInputs();
	
	pipeline::Value<Blocks> needBlocks;
	boost::shared_ptr<Blocks> sliceBlocks = boost::make_shared<Blocks>(_blocks);
	
	boost::shared_ptr<SliceReader> sliceReader = boost::make_shared<SliceReader>();
	pipeline::Value<Slices> slices;
	boost::shared_ptr<Segments> segments = boost::make_shared<Segments>();
	boost::shared_ptr<SegmentWriter> segmentWriter = boost::make_shared<SegmentWriter>();

	unsigned int zBegin = _blocks->location().z;
	unsigned int zEnd = zBegin + _blocks->size().z;
	
	bool segmentsCached = true;
	
	// Expand sliceBlocks by z+1. We need to grab Slices from the first section of the next block
	sliceBlocks->expand(util::point3<int>(0, 0, 1));
	
	// Check to see if we have any blocks for which the extraction is necessary.
	foreach (boost::shared_ptr<Block> block, *_blocks)
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
	
	sliceReader->setInput("blocks", sliceBlocks);
	sliceReader->setInput("store", _sliceStore);
	slices = sliceReader->getOutput("slices");
	
	LOG_DEBUG(segmentguarantorlog) << "Read " << slices->size() <<
		" slices from requested geometry. Expanding blocks to fit." << std::endl;
	
	sliceBlocks = _blocks->getManager()->blocksInBox(boundingBox(slices));

	// Check again
	if (!checkBlockSlices(sliceBlocks, needBlocks))
	{
		return needBlocks;
	}

	sliceReader->setInput("blocks", sliceBlocks);
	slices = sliceReader->getOutput("slices");

	
	for (unsigned int z = zBegin; z < zEnd; ++z)
	{
		// If sections z and z + 1 exist in our whitelist
		if (_blocks->getManager()->isValidZ(z) && _blocks->getManager()->isValidZ(z + 1))
		{
			pipeline::Value<Segments> extractedSegments;
			
			boost::shared_ptr<SegmentExtractor> extractor = boost::make_shared<SegmentExtractor>();
			// Collect slices for sections z and z + 1
			boost::shared_ptr<Slices> prevSlices = collectSlicesByZ(slices, z);
			boost::shared_ptr<Slices> nextSlices = collectSlicesByZ(slices, z + 1);
			
			// Set up the extractor
			extractor->setInput("previous slices", prevSlices);
			extractor->setInput("next slices", nextSlices);
			
			// and grab the segments.
			extractedSegments = extractor->getOutput("segments");
			
			LOG_DEBUG(segmentguarantorlog) << "Got " << extractedSegments->size() << " segments"
				<< std::endl;
			
			segments->addAll(extractedSegments);
		}
	}
	
	segmentWriter->setInput("segments", segments);
	segmentWriter->setInput("blocks", _blocks);
	segmentWriter->setInput("store", _segmentStore);
	
	if (_rawImageStore)
	{
		guaranteeFeatures(segmentWriter, segments);
	}
	
	segmentWriter->writeSegments();
	
	foreach (boost::shared_ptr<Block> block, *_blocks)
	{
		block->setSegmentsFlag(true);
	}
	
	// needBlocks should be empty
	return needBlocks;
}


void SegmentGuarantor::updateOutputs()
{
	*_needBlocks = *guaranteeSegments();
}

boost::shared_ptr<Slices> SegmentGuarantor::collectSlicesByZ(
	const boost::shared_ptr<Slices> slices, unsigned int z) const
{
	boost::shared_ptr<Slices> zSlices = boost::make_shared<Slices>();
	
	foreach (boost::shared_ptr<Slice> slice, *slices)
	{
		if (slice->getSection() == z)
		{
			zSlices->add(slice);
			zSlices->setConflicts(slice->getId(), slices->getConflicts(slice->getId()));
		}
	}
	
	LOG_DEBUG(segmentguarantorlog) << "Collected " << zSlices->size() << " slices for z=" << z << std::endl;
	
	return zSlices;
}

boost::shared_ptr<Box<> >
SegmentGuarantor::boundingBox(const boost::shared_ptr<Slices> slices)
{
	boost::shared_ptr<Box<> > box;
	
	if (slices->size() == 0)
	{
		box = boost::make_shared<Box<> >();
	}
	else
	{
		util::rect<unsigned int> bound = (*slices)[0]->getComponent()->getBoundingBox();
		unsigned int zMax = (*slices)[0]->getSection();
		unsigned int zMin = zMax;
		
		foreach (const boost::shared_ptr<Slice> slice, *slices)
		{
			bound.fit(slice->getComponent()->getBoundingBox());
			
			if (zMax < slice->getSection())
			{
				zMax = slice->getSection();
			}
			
			if (zMin > slice->getSection())
			{
				zMin = slice->getSection();
			}
		}
		
		box = boost::make_shared<Box<> >(bound, zMin, zMax - zMin);
	}
	
	return box;
}

bool
SegmentGuarantor::checkBlockSlices(const boost::shared_ptr<Blocks> sliceBlocks,
								   const boost::shared_ptr<Blocks> needBlocks)
{
	bool ok = true;
	foreach (boost::shared_ptr<Block> block, *sliceBlocks)
	{
		if (!block->getSlicesFlag())
		{
			ok = false;
			needBlocks->add(block);
		}
	}
	
	return ok;
}

boost::shared_ptr<Blocks>
SegmentGuarantor::segmentBoundingBlocks(const boost::shared_ptr<Segments> inSegments)
{
	if (inSegments->size() == 0)
	{
		return boost::make_shared<Blocks>();
	}
	else
	{
		std::vector<boost::shared_ptr<Segment> > segments = inSegments->getSegments();
		util::rect<int> bound(segments[0]->getSlices()[0]->getComponent()->getBoundingBox());
		boost::shared_ptr<Box<> > box;
		boost::shared_ptr<Blocks> blocks;
		
		foreach (boost::shared_ptr<Segment> segment, segments)
		{
			foreach (boost::shared_ptr<Slice> slice, segment->getSlices())
			{
				bound.fit(slice->getComponent()->getBoundingBox());
			}
		}
		
		box = boost::make_shared<Box<> >(bound, _blocks->location().z, _blocks->size().z);
		blocks = _blocks->getManager()->blocksInBox(box);
		return blocks;
	}
}

void
SegmentGuarantor::guaranteeFeatures(const boost::shared_ptr<SegmentWriter> segmentWriter,
									const boost::shared_ptr<Segments> segments)
{
	boost::shared_ptr<Blocks> blocks = segmentBoundingBlocks(segments);
	pipeline::Value<util::point3<unsigned int> > offset(blocks->location());
	boost::shared_ptr<SegmentFeaturesExtractor> featuresExtractor =
		boost::make_shared<SegmentFeaturesExtractor>();
	pipeline::Value<Features> features;
	
	featuresExtractor->setInput("segments", segments);
	featuresExtractor->setInput("raw sections", _rawImageStore->getImageStack(*blocks));
	featuresExtractor->setInput("crop offset", offset);
	
	features = featuresExtractor->getOutput("all features");
	
	segmentWriter->setInput("features", features);
}


