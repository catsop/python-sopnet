#include "SliceGuarantor.h"

#include <map>
#include <set>
#include <imageprocessing/ImageExtractor.h>
#include <sopnet/sopnet/slices/SliceExtractor.h>
#include <sopnet/sopnet/slices/Slice.h>
#include <util/rect.hpp>
#include <util/Logger.h>
#include <util/foreach.h>
#include <pipeline/Process.h>
#include <pipeline/Value.h>

logger::LogChannel sliceguarantorlog("sliceguarantorlog", "[SliceGuarantor] ");

Blocks
SliceGuarantor::guaranteeSlices(const Blocks& blocks)
{
	Blocks extractBlocks = Blocks();
	Slices slices;
	ConflictSets conflictSets;
	
	LOG_DEBUG(sliceguarantorlog) << "Guaranteeing slices for region " << blocks << std::endl;
	
	if (checkSlices(blocks))
	{
		LOG_DEBUG(sliceguarantorlog) << "All blocks have already been extracted" << std::endl;
		return extractBlocks;
	}
	
	LOG_ALL(sliceguarantorlog) << "The given blocks have not yet had slices extracted" <<
		std::endl;

	// Slices and ConflictSets extracted from the image underlying the requested area.
	// This is done section-by-section, so a vector is used in order to make it easier
	// to multi-thread this later.
	
	// extracted Slices.
	std::vector<Slices> slicesVector(blocks.size().z);
	// extracted conflict sets
	std::vector<ConflictSets> conflictSetsVector(blocks.size().z); 
	
	// This isn't *really* true.
	bool allBad = true;

	// Extract slices independently by z.
	for (unsigned int i = 0; i < blocks.size().z; ++i)
	{
		unsigned int z = i + blocks.location().z;
		Slices zSlices;
		ConflictSets zConflict;
		Blocks zBlocks;
		
		allBad = !extractSlices(z, zSlices, zConflict, zBlocks, blocks) && allBad;
		
		slicesVector[i] = zSlices;
		conflictSetsVector[i] = zConflict;
		
		extractBlocks.addAll(zBlocks);
	}

	// If all sections yielded empty images, then we need to regenerate the image stack.
	if (allBad)
	{
		LOG_DEBUG(sliceguarantorlog) << "No images found from which to extract slices" << std::endl;
		return extractBlocks;
	}
	
	for (unsigned int i = 0; i < blocks.size().z; ++i)
	{
		slices.addAll(slicesVector[i]);
		conflictSets.addAll(conflictSetsVector[i]);
	}
	
	LOG_DEBUG(sliceguarantorlog) << "Writing " << slices.size() << " slices to " <<
		extractBlocks.length() << " blocks" << std::endl;
	
	_sliceStore->writeSlices(slices, conflictSets, extractBlocks);
	
	foreach (boost::shared_ptr<Block> block, blocks)
	{
		block->setSlicesFlag(true);
	}
	
	LOG_DEBUG(sliceguarantorlog) << "Done." << std::endl;
	
	return Blocks();
}

void
SliceGuarantor::setMserParameters(const boost::shared_ptr<MserParameters> mserParameters)
{
	_mserParameters = mserParameters;
}

void
SliceGuarantor::setSliceStore(const boost::shared_ptr<SliceStore> sliceStore)
{
	_sliceStore = sliceStore;
}

void
SliceGuarantor::setMaximumArea(const unsigned int area)
{
	_maximumArea = boost::make_shared<unsigned int>(area);
}

void
SliceGuarantor::setStackStore(const boost::shared_ptr<StackStore> stackStore)
{
	_stackStore = stackStore;
}

/**
 * Checks whether slices have already been extracted for our Block request.
 */
bool
SliceGuarantor::checkSlices(const Blocks& blocks)
{
	// Check to see whether each block in guaranteeBlocks has already had its slices extracted.
	// If this is the case, we have no work to do
	foreach (boost::shared_ptr<Block> block, blocks)
	{
		if (!block->getSlicesFlag())
		{
			return false;
		}
	}
	
	return true;
	
}

bool
SliceGuarantor::sizeOk(util::point3<unsigned int> size)
{
	if (_maximumArea)
	{
		return size.x * size.y < *_maximumArea;
	}
	else
	{
		return true;
	}
}

bool
SliceGuarantor::extractSlices(const unsigned int z,
							  Slices& slices,
							  ConflictSets& conflictSets,
							  Blocks& extractBlocks,
							  const Blocks& requestBlocks)
{
	LOG_ALL(sliceguarantorlog) << "Setting up mini pipeline for section " << z << std::endl;
	Blocks nbdBlocks;
	util::rect<unsigned int> blocksRect = requestBlocks;
	
	bool okSlices = false;
	pipeline::Process<SliceExtractor<unsigned char> > sliceExtractor(z, true);

	pipeline::Value<Slices> slicesValue;
	pipeline::Value<ConflictSets> conflictValue;
	
	extractBlocks.addAll(requestBlocks);
	// Dilate once beforehand.
	extractBlocks.dilate(1, 1, 0);

	while (!okSlices && sizeOk(extractBlocks.size()))
	{
		util::rect<unsigned int> bound = extractBlocks;
		util::point<int> translate(extractBlocks.location().x, extractBlocks.location().y);
		Box<> box(bound, z, 1);
		boost::shared_ptr<Image> image = (*_stackStore->getImageStack(box))[0];
		
		LOG_ALL(sliceguarantorlog) << "Processing over " << bound << std::endl;
		
		if (image->width() * image->height() == 0)
		{
			return false;
		}
		
		nbdBlocks = Blocks(extractBlocks);

		sliceExtractor->setInput("membrane", image);

		if (_mserParameters)
		{
			sliceExtractor->setInput("mser parameters", _mserParameters);
		}
			
		slicesValue = sliceExtractor->getOutput("slices");
		conflictValue = sliceExtractor->getOutput("conflict sets");
		
		LOG_DEBUG(sliceguarantorlog) << "Extracted " << slicesValue->size() << " slices" << std::endl;
		
		// Slices are extracted in [0 0 w h]. Translate them to Box coordinates.
		LOG_ALL(sliceguarantorlog) << "Translating slices to request coordinates" << std::endl;
		foreach(boost::shared_ptr<Slice> slice, *slicesValue)
		{
			slice->translate(translate);
			if (blocksRect.intersects(
				static_cast<util::rect<unsigned int> >(slice->getComponent()->getBoundingBox())))
			{
				checkWhole(*slice, extractBlocks, nbdBlocks);
			}
		}
		
		LOG_ALL(sliceguarantorlog) << "Extract: " << extractBlocks << ", Neighbor: " <<
			nbdBlocks << std::endl; 
		
		if (!(okSlices = extractBlocks.size() == nbdBlocks.size()))
		{
			LOG_ALL(sliceguarantorlog) << "Need to expand" << std::endl;
			extractBlocks.addAll(nbdBlocks);
		}
		else
		{
			LOG_ALL(sliceguarantorlog) << "Done for section " << z << std::endl;
		}
	}
	
	conflictSets.addAll(*conflictValue);
	collectOutputSlices(*slicesValue, *conflictValue, slices, requestBlocks);
	
	return true;
}

/**
 * Collect all of the Slice's that we need to store from extractedSlices and put
 * them into slices.
 */
void
SliceGuarantor::collectOutputSlices(const Slices& extractedSlices,
								   const ConflictSets& extractedConflict,
								   Slices& slices,
								   const Blocks& blocks)
{
	std::map<unsigned int, boost::shared_ptr<Slice> > idSliceMap;
	std::set<unsigned int> idSet;
	// Must be separate from idSet to avoid problems with the possibility of multiply-overlapping
	// conflict sets.
	std::set<unsigned int> conflictCliqueIdSet;
	util::rect<unsigned int> blocksRect = blocks;
	
	// Collection in two steps:
	// 1) Find all Slice's that overlap the guarantee blocks
	// 2) Find all Slice's that are in a conflict set with any of the Slice's found in (1)
	
	foreach (boost::shared_ptr<Slice> slice, extractedSlices)
	{
		// Map slice id to slice object
		idSliceMap[slice->getId()] = slice;
		
		// If the slice overlaps the guarantee blocks, add it to the set
		if (blocksRect.intersects(
			static_cast<util::rect<unsigned int> >(slice->getComponent()->getBoundingBox())))
		{
			idSet.insert(slice->getId());
		}
	}
	
	foreach (const ConflictSet conflictSet, extractedConflict)
	{
		// If the conflictSet contains any of the id's, add all of them to the conflictCliqueIdSet
		if (containsAny(conflictSet, idSet))
		{
			conflictCliqueIdSet.insert(conflictSet.getSlices().begin(),
									   conflictSet.getSlices().end());
		}
	}
	
	// Merge the sets.
	idSet.insert(conflictCliqueIdSet.begin(), conflictCliqueIdSet.end());
	
	foreach (unsigned int id, idSet)
	{
		slices.add(idSliceMap[id]);
	}
}

bool
SliceGuarantor::containsAny(const ConflictSet& conflictSet, const std::set<unsigned int>& idSet)
{
	foreach (const unsigned int id, conflictSet.getSlices())
	{
		if (idSet.count(id))
		{
			return true;
		}
	}
	
	return false;
}

void
SliceGuarantor::checkWhole(const Slice& slice,
						   const Blocks& extractBlocks,
						   Blocks& nbdBlocks) const
{
	util::rect<unsigned int> sliceBound = slice.getComponent()->getBoundingBox();
	util::rect<unsigned int> blockBound = extractBlocks;
	
	LOG_ALL(sliceguarantorlog) << "block bound: " << blockBound << ", slice bound: " <<
		sliceBound << " for slice " << slice.getId() << std::endl;
	
	if (sliceBound.minX <= blockBound.minX)
	{
		Blocks expandBlocks = Blocks(extractBlocks);
		expandBlocks.expand(util::point3<int>(-1, 0, 0));
		nbdBlocks.addAll(expandBlocks);
		LOG_ALL(sliceguarantorlog) << "Slice touches -x boundary" << std::endl;
	}
	
	if (sliceBound.maxX >= blockBound.maxX)
	{
		Blocks expandBlocks = Blocks(extractBlocks);
		expandBlocks.expand(util::point3<int>(1, 0, 0));
		nbdBlocks.addAll(expandBlocks);
		LOG_ALL(sliceguarantorlog) << "Slice touches +x boundary" << std::endl;
	}

	if (sliceBound.minY <= blockBound.minY)
	{
		Blocks expandBlocks = Blocks(extractBlocks);
		expandBlocks.expand(util::point3<int>(0, -1, 0));
		nbdBlocks.addAll(expandBlocks);
		LOG_ALL(sliceguarantorlog) << "Slice touches -y boundary" << std::endl;
	}
	
	if (sliceBound.maxY >= blockBound.maxY)
	{
		Blocks expandBlocks = Blocks(extractBlocks);
		expandBlocks.expand(util::point3<int>(0, 1, 0));
		nbdBlocks.addAll(expandBlocks);
		LOG_ALL(sliceguarantorlog) << "Slice touches +y boundary" << std::endl;
	}
}

