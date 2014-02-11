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
	registerInput(_forceExplanation, "force explanation");
	
	registerOutput(_result, "count");
}

boost::shared_ptr<Slices>
SegmentGuarantor::collectNecessarySlices(const boost::shared_ptr<SliceReader>& sliceReader,
										 const boost::shared_ptr< Blocks >& sliceBlocks)
{
	pipeline::Value<Slices> slices, conflictSlices;
	boost::shared_ptr<Blocks> extraBlocks = boost::make_shared<Blocks>(sliceBlocks);
// 	boost::shared_ptr<ComponentTreeExtractor> componentTreeExtractor =
// 		boost::make_shared<ComponentTreeExtractor>();
	
	
	// The Slice Reader pulls in the slices associated with sliceBlocks,
	// along with all descendants, so we should have everything we need.
	sliceReader->setInput("blocks", sliceBlocks);
	sliceReader->setInput("store", _sliceStore);
	

	// Set conflict data in slices
// 	componentTreeExtractor->setInput("blocks", extraBlocks);
// 	componentTreeExtractor->setInput("slices", sliceReader->getOutput("slices"));
// 	componentTreeExtractor->setInput("force explanation", _forceExplanation);
// 	componentTreeExtractor->setInput("store", _sliceStore);
	
// 	conflictSlices = componentTreeExtractor->getOutput("slices");
	
	return conflictSlices;
}


void SegmentGuarantor::guaranteeSegments(
	const boost::shared_ptr<Blocks>& guaranteeBlocks,
	const boost::shared_ptr<Blocks>& sliceBlocks)
{
	boost::shared_ptr<SliceReader> sliceReader = boost::make_shared<SliceReader>();
	boost::shared_ptr<LinearConstraints> emptyConstraints =
		boost::make_shared<LinearConstraints>();
	boost::shared_ptr<Slices> slices;
	//boost::shared_ptr<LinearConstraints> segmentConstraints;
	boost::shared_ptr<Segments> segments = boost::make_shared<Segments>();
	boost::shared_ptr<SegmentWriter> segmentWriter = boost::make_shared<SegmentWriter>();
	boost::shared_ptr<Blocks> extraBlocks;
	pipeline::Value<SegmentStoreResult> result;
	
	unsigned int zBegin = guaranteeBlocks->location().z;
	unsigned int zEnd = zBegin + guaranteeBlocks->size().z;
	
	slices = collectNecessarySlices(sliceReader, sliceBlocks);
	
	LOG_DEBUG(segmentguarantorlog) << "Read " << slices->size() << " slices." << std::endl;
	
	for (unsigned int z = zBegin; z < zEnd; ++z)
	{
		if (_blockManager->isValidZ(z) && _blockManager->isValidZ(z + 1))
		{
			pipeline::Value<Segments> extractedSegments;
			
			boost::shared_ptr<SegmentExtractor> extractor = boost::make_shared<SegmentExtractor>();
			boost::shared_ptr<Slices> prevSlices = collectSlicesByZ(slices, z);
			boost::shared_ptr<Slices> nextSlices = collectSlicesByZ(slices, z + 1);
			
			foreach (boost::shared_ptr<Slice> slice, *prevSlices)
			{
				LOG_DEBUG(segmentguarantorlog) << "Slice " << slice->getId() << " has conflicts in prev:";
				foreach (unsigned int conflictId, prevSlices->getConflicts(slice->getId()))
				{
					LOG_DEBUG(segmentguarantorlog) << " " << conflictId;
				}
				LOG_DEBUG(segmentguarantorlog) << std::endl;
				
				LOG_DEBUG(segmentguarantorlog) << "Slice " << slice->getId() << " has conflicts in conf:";
				foreach (unsigned int conflictId, slices->getConflicts(slice->getId()))
				{
					LOG_DEBUG(segmentguarantorlog) << " " << conflictId;
				}
				LOG_DEBUG(segmentguarantorlog) << std::endl;
			}
			
			extractor->setInput("previous slices", prevSlices);
			extractor->setInput("next slices", nextSlices);
			
			extractedSegments = extractor->getOutput("segments");
			
			LOG_DEBUG(segmentguarantorlog) << "Got " << extractedSegments->size() << " segments"
				<< std::endl;
			
			segments->addAll(extractedSegments);
		}
	}
	
	segmentWriter->setInput("segments", segments);
	segmentWriter->setInput("box", guaranteeBlocks);
	segmentWriter->setInput("block manager", _blockManager);
	segmentWriter->setInput("store", _segmentStore);
	
	result = segmentWriter->getOutput("count");
	*_result = *result;
}


void SegmentGuarantor::updateOutputs()
{
	boost::shared_ptr<Blocks> guaranteeBlocks = _blocks;
	boost::shared_ptr<Blocks> sliceBlocks = boost::make_shared<Blocks>(guaranteeBlocks);
	bool allExtracted = true;
	
	// Check whether this update needs to occur.
	foreach (boost::shared_ptr<Block> block, *guaranteeBlocks)
	{
		allExtracted = block->setSegmentsFlag(true) && allExtracted;
	}
	
	if (!allExtracted)
	{
		
		// We need the slices across the +z boundary, in order to ensure that we'll extract all
		// of the required segments.
		sliceBlocks->expand(util::point3<int>(0, 0, 1));

		// Check that we have slices in all of the necessary blocks.
		foreach (boost::shared_ptr<Block> block, *sliceBlocks)
		{
			if (!block->getSlicesFlag())
			{
				boost::shared_ptr<SegmentStoreResult> result = 
					boost::make_shared<SegmentStoreResult>();
				*_result = *result;
				LOG_DEBUG(segmentguarantorlog) << "Block at " << block->location() <<
					" has no slices. Cannot proceed" << std::endl;
				return;
			}
		}
		
		guaranteeSegments(guaranteeBlocks, sliceBlocks);
	}
	else
	{
		LOG_DEBUG(segmentguarantorlog) << "Segments already in cache for all requested Blocks" <<
			std::endl;
	}
	
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

