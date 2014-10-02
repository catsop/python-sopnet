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

SliceGuarantor::SliceGuarantor(
			const ProjectConfiguration&   projectConfiguration,
			boost::shared_ptr<SliceStore> sliceStore,
			boost::shared_ptr<StackStore> stackStore) :
	_sliceStore(sliceStore),
	_stackStore(stackStore),
	_blockUtils(projectConfiguration) {}

Blocks
SliceGuarantor::guaranteeSlices(const Blocks& blocks)
{
	Blocks extractBlocks = Blocks();
	Slices slices;
	ConflictSets conflictSets;
	
	LOG_DEBUG(sliceguarantorlog) << "Guaranteeing slices for blocks " << blocks << std::endl;
	
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

	const util::box<unsigned int>& blockBoundingBox = _blockUtils.getBoundingBox(blocks);
	unsigned int firstSection = blockBoundingBox.min.z;
	unsigned int numSections  = blockBoundingBox.depth();

	// extracted Slices.
	std::vector<Slices> slicesVector(numSections);
	// extracted conflict sets
	std::vector<ConflictSets> conflictSetsVector(numSections); 

	// Extract slices independently by z.
	for (unsigned int i = 0; i < numSections; ++i)
	{
		unsigned int z = firstSection + i;
		Slices zSlices;
		ConflictSets zConflict;
		Blocks zBlocks;

		extractSlices(z, zSlices, zConflict, zBlocks, blocks);

		slicesVector[i] = zSlices;
		conflictSetsVector[i] = zConflict;

		extractBlocks.addAll(zBlocks);
	}

	for (unsigned int i = 0; i < numSections; ++i)
	{
		slices.addAll(slicesVector[i]);
		conflictSets.addAll(conflictSetsVector[i]);
	}
	
	LOG_DEBUG(sliceguarantorlog) << "Writing " << slices.size() << " slices to " <<
		extractBlocks.size() << " blocks" << std::endl;

	writeSlicesAndConflicts(slices, conflictSets, blocks);

	LOG_DEBUG(sliceguarantorlog) << "Done." << std::endl;

	return Blocks();
}

void
SliceGuarantor::writeSlicesAndConflicts(
		const Slices&       slices,
		const ConflictSets& conflictSets,
		const Blocks&       blocks) {

	foreach (const Block& block, blocks) {

		boost::shared_ptr<Slices>       blockSlices       = collectSlicesByBlock(slices, block);
		boost::shared_ptr<ConflictSets> blockConflictSets = collectConflictBySlices(conflictSets, *blockSlices);

		_sliceStore->associateSlicesToBlock(*blockSlices, block);
		_sliceStore->associateConflictSetsToBlock(*blockConflictSets, block);
	}
}

boost::shared_ptr<Slices>
SliceGuarantor::collectSlicesByBlock(const Slices& slices, const Block& block) {

	util::rect<unsigned int> blockRect = _blockUtils.getBoundingBox(block).project_xy();

	boost::shared_ptr<Slices> blockSlices = boost::make_shared<Slices>();

	foreach (boost::shared_ptr<Slice> slice, slices) {

		util::rect<unsigned int> sliceBoundingBox = slice->getComponent()->getBoundingBox();

		if (blockRect.intersects(sliceBoundingBox))
			blockSlices->add(slice);
	}

	return blockSlices;
}

boost::shared_ptr<ConflictSets>
SliceGuarantor::collectConflictBySlices(const ConflictSets& conflictSets, const Slices& slices) {

	boost::shared_ptr<ConflictSets> sliceConflictSets = boost::make_shared<ConflictSets>();

	foreach (ConflictSet conflictSet, conflictSets)
		if (containsAny(conflictSet, slices))
			sliceConflictSets->add(conflictSet);

	return sliceConflictSets;
}

bool
SliceGuarantor::containsAny(const ConflictSet& conflictSet, const Slices& slices) {

	foreach (const boost::shared_ptr<Slice> slice, slices)
		if (conflictSet.getSlices().count(slice->getId()))
			return true;

	return false;
}

void
SliceGuarantor::setMserParameters(const boost::shared_ptr<MserParameters> mserParameters)
{
	_mserParameters = mserParameters;
}

/**
 * Checks whether slices have already been extracted for our Block request.
 */
bool
SliceGuarantor::checkSlices(const Blocks& blocks)
{
	// Check to see whether each block has already had its slices extracted.
	// If this is the case, we have no work to do
	foreach (const Block& block, blocks) {
		if (!_sliceStore->getSlicesFlag(block))
			return false;
	}

	return true;
}

void
SliceGuarantor::extractSlices(const unsigned int z,
							  Slices& slices,
							  ConflictSets& conflictSets,
							  Blocks& extractBlocks,
							  const Blocks& requestBlocks)
{
	LOG_ALL(sliceguarantorlog) << "Setting up mini pipeline for section " << z << std::endl;
	Blocks nbdBlocks;
	util::rect<unsigned int> blocksRect = _blockUtils.getBoundingBox(requestBlocks).project_xy();

	bool okSlices = false;
	pipeline::Process<SliceExtractor<unsigned char> > sliceExtractor(z, true);

	pipeline::Value<Slices> slicesValue;
	pipeline::Value<ConflictSets> conflictValue;

	extractBlocks.addAll(requestBlocks);
	// Dilate once beforehand.
	_blockUtils.expand(
			extractBlocks,
			-1, -1, 0,
			 1,  1, 0);

	while (!okSlices)
	{
		util::box<unsigned int> boundingBox = _blockUtils.getBoundingBox(extractBlocks);

		util::rect<unsigned int> bound = boundingBox.project_xy();
		util::point<int> translate(boundingBox.min.x, boundingBox.min.y);
		util::box<unsigned int> box(bound.minX, bound.minY, z, bound.maxX, bound.maxY, z + 1);
		boost::shared_ptr<Image> image = (*_stackStore->getImageStack(box))[0];

		LOG_ALL(sliceguarantorlog) << "Processing over " << bound << std::endl;

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

		util::rect<unsigned int> extractBlocksSize = _blockUtils.getBoundingBox(extractBlocks).project_xy();
		util::rect<unsigned int> nbdBlocksSize     = _blockUtils.getBoundingBox(nbdBlocks).project_xy();

		// if the bounding rectangle of the blocks we extracted slices for has 
		// the same size as the one for the blocks we should extract, we are 
		// done
		okSlices = (extractBlocksSize == nbdBlocksSize);

		if (!okSlices)
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
	util::rect<unsigned int> blocksRect = _blockUtils.getBoundingBox(blocks).project_xy();
	
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
	util::rect<unsigned int> blockBound = _blockUtils.getBoundingBox(extractBlocks).project_xy();
	
	LOG_ALL(sliceguarantorlog) << "block bound: " << blockBound << ", slice bound: " <<
		sliceBound << " for slice " << slice.getId() << std::endl;
	
	if (sliceBound.minX <= blockBound.minX)
	{
		Blocks expandBlocks = Blocks(extractBlocks);
		_blockUtils.expand(
				expandBlocks,
				0, 0, 0,
				1, 0, 0);
		nbdBlocks.addAll(expandBlocks);
		LOG_ALL(sliceguarantorlog) << "Slice touches -x boundary" << std::endl;
	}
	
	if (sliceBound.maxX >= blockBound.maxX)
	{
		Blocks expandBlocks = Blocks(extractBlocks);
		_blockUtils.expand(
				expandBlocks,
				1, 0, 0,
				0, 0, 0);
		nbdBlocks.addAll(expandBlocks);
		LOG_ALL(sliceguarantorlog) << "Slice touches +x boundary" << std::endl;
	}

	if (sliceBound.minY <= blockBound.minY)
	{
		Blocks expandBlocks = Blocks(extractBlocks);
		_blockUtils.expand(
				expandBlocks,
				0, 0, 0,
				0, 1, 0);
		nbdBlocks.addAll(expandBlocks);
		LOG_ALL(sliceguarantorlog) << "Slice touches -y boundary" << std::endl;
	}
	
	if (sliceBound.maxY >= blockBound.maxY)
	{
		Blocks expandBlocks = Blocks(extractBlocks);
		_blockUtils.expand(
				expandBlocks,
				0, 1, 0,
				0, 0, 0);
		nbdBlocks.addAll(expandBlocks);
		LOG_ALL(sliceguarantorlog) << "Slice touches +y boundary" << std::endl;
	}
}

