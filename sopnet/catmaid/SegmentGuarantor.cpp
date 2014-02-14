#include "SegmentGuarantor.h"
#include <util/Logger.h>
#include <catmaid/persistence/SegmentWriter.h>
#include <sopnet/segments/SegmentExtractor.h>
#include <pipeline/Value.h>

logger::LogChannel segmentguarantorlog("segmentguarantorlog", "[SegmentGuarantor] ");

SegmentGuarantor::SegmentGuarantor()
{
	registerInput(_blocks, "blocks");
	registerInput(_segmentStore, "segment store");
	registerInput(_sliceStore, "slice store");
	
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
	foreach (boost::shared_ptr<Block> block, *sliceBlocks)
	{
		if (!block->getSlicesFlag())
		{
			needBlocks->add(block);
		}
	}
	
	// If not, return a list of blocks that should have slices extracted before proceeding.
	if (!needBlocks->empty())
	{
		return needBlocks;
	}
	
	// OK, here we go.
	// Note that we don't have to go searching around for Slices in other Block's, because
	// the SliceReader guarantees (by way of the guarantee provided at the SliceStore level)
	// that it will return fully-populated conflict-set cliques of Slices.
	sliceReader->setInput("blocks", sliceBlocks);
	sliceReader->setInput("store", _sliceStore);
	slices = sliceReader->getOutput("slices");
	
	LOG_DEBUG(segmentguarantorlog) << "Read " << slices->size() << " slices." << std::endl;
	
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
	
	
	
}

boost::shared_ptr<Slices> SegmentGuarantor::collectSlicesByZ(
	const boost::shared_ptr< Slices >& slices, unsigned int z) const
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

