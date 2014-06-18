#include "SegmentGuarantor.h"
#include <util/Logger.h>
#include <sopnet/segments/SegmentExtractor.h>
#include <features/SegmentFeaturesExtractor.h>
#include <pipeline/Value.h>
#include <catmaid/EndExtractor.h>

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
	boost::shared_ptr<Blocks> inputBlocks = _blocks;
	boost::shared_ptr<Blocks> sliceBlocks = boost::make_shared<Blocks>(inputBlocks);
	
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
		" slices from blocks " << *sliceBlocks << ". Expanding blocks to fit." << std::endl;
	
	sliceBlocks = _blocks->getManager()->blocksInBox(slicesBoundingBox(slices));

	// Check again
	if (!checkBlockSlices(sliceBlocks, needBlocks))
	{
		return needBlocks;
	}

	sliceReader->setInput("blocks", sliceBlocks);
	slices = sliceReader->getOutput("slices");

	LOG_DEBUG(segmentguarantorlog) << "Altogether, I have " << slices->size() << " slices" << std::endl;
	
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

	// In the case that the guarantee block shares its upper bound with the whole stack, extracte
	// EndSegments there and write them to the store.
	// zEnd is not strictly in the z-range of the stack bounds, so we subtract 1
	/*if (_blocks->getManager()->isUpperBound(zEnd - 1))
	{
		boost::shared_ptr<EndExtractor> endExtractor = boost::make_shared<EndExtractor>();
		endExtractor->setInput("segments", segments);
		endExtractor->setInput("slices", slices);
		segmentWriter->setInput("segments", endExtractor->getOutput());
	}
	else
	{*/
		segmentWriter->setInput("segments", segments);
// 	}
	
	segmentWriter->setInput("blocks", _blocks);
	segmentWriter->setInput("store", _segmentStore);
	
	if (_rawImageStore.isSet())
	{
		segmentWriter->setInput("features", guaranteeFeatures(segments));
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
	_needBlocks = new Blocks();
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
SegmentGuarantor::slicesBoundingBox(const boost::shared_ptr<Slices> slices)
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

pipeline::Value<Features>
SegmentGuarantor::guaranteeFeatures(const boost::shared_ptr<Segments> segments)
{
	boost::shared_ptr<Box<> > box = segments->boundingBox();
	boost::shared_ptr<Blocks> blocks = _blocks->getManager()->blocksInBox(box);
	pipeline::Value<util::point3<unsigned int> > offset(blocks->location());
	boost::shared_ptr<SegmentFeaturesExtractor> featuresExtractor =
		boost::make_shared<SegmentFeaturesExtractor>();
	pipeline::Value<Features> features;
	
	LOG_DEBUG(segmentguarantorlog) << "Extracting features for " << segments->size() <<
		" segments" << std::endl;
	
	featuresExtractor->setInput("segments", segments);
	featuresExtractor->setInput("raw sections", _rawImageStore->getImageStack(*blocks));
	featuresExtractor->setInput("crop offset", offset);
	
	features = featuresExtractor->getOutput("all features");
	
	LOG_DEBUG(segmentguarantorlog) << "Done extracting features" << std::endl;
	
	return features;
}


