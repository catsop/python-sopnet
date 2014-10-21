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

void
SliceGuarantor::setMserParameters(const boost::shared_ptr<MserParameters> mserParameters) {

	_mserParameters = mserParameters;
}

Blocks
SliceGuarantor::guaranteeSlices(const Blocks& requestedBlocks) {

	// have the slices been extracted already?
	if (checkSlices(requestedBlocks))
		return Blocks();

	// get the number of sections
	util::box<unsigned int> blockBoundingBox = _blockUtils.getBoundingBox(requestedBlocks);
	unsigned int firstSection = blockBoundingBox.min.z;
	unsigned int numSections  = blockBoundingBox.depth();

	// slices and conflict sets for the requested block
	Slices       slices;
	ConflictSets conflictSets;

	// non-request blocks that were needed to extract complete slices
	Blocks       expansionBlocks;

	// get the slices and conflict sets for each section
	for (unsigned int section = firstSection; section < firstSection + numSections; section++)
		expansionBlocks.addAll(
				extractSlicesAndConflicts(slices, conflictSets, requestedBlocks, section));

	// store them
	writeSlicesAndConflicts(
			slices,
			conflictSets,
			requestedBlocks,
			expansionBlocks);

	return Blocks();
}

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

Blocks
SliceGuarantor::extractSlicesAndConflicts(
		Slices&            slices,
		ConflictSets&      conflictSets,
		const Blocks&      requestedBlocks,
		const unsigned int z) {

	LOG_ALL(sliceguarantorlog) << "extracting slices in section " << z << std::endl;

	// there will be slices that touch the outline of requestedBlocks
	// -- expand once in x and y
	Blocks expansionBlocks = requestedBlocks;
	_blockUtils.expand(
			expansionBlocks,
			 1,  1, 0,
			 1,  1, 0);

	pipeline::Process<SliceExtractor<unsigned char> > sliceExtractor(z, true);
	pipeline::Value<Slices>                           slicesValue;
	pipeline::Value<ConflictSets>                     conflictsValue;

	// subset of slices and conflict sets that have to be extracted completely
	Slices       requiredSlices;
	ConflictSets requiredConflictSets;

	// are all required slices complete?
	bool slicesComplete = false;
	while (!slicesComplete) {

		// 2D bonding box of expansion blocks
		util::rect<unsigned int> bound = _blockUtils.getBoundingBox(expansionBlocks).project_xy();

		// box for only the current section
		util::box<unsigned int> sectionBox(bound.minX, bound.minY, z, bound.maxX, bound.maxY, z + 1);

		// get the image for this box
		boost::shared_ptr<Image> image = (*_stackStore->getImageStack(sectionBox))[0];

		LOG_ALL(sliceguarantorlog) << "Processing over " << bound << std::endl;

		// extract slices and conflict sets
		sliceExtractor->setInput("membrane", image);
		if (_mserParameters)
			sliceExtractor->setInput("mser parameters", _mserParameters);

		slicesValue = sliceExtractor->getOutput("slices");
		conflictsValue = sliceExtractor->getOutput("conflict sets");
		
		LOG_DEBUG(sliceguarantorlog) << "Extracted " << slicesValue->size() << " slices" << std::endl;

		LOG_ALL(sliceguarantorlog) << "Translating slices to request coordinates" << std::endl;

		// Slices are extracted in [0 0 w h]. Translate them to section 
		// coordinates.
		std::map<SliceHash, SliceHash> sliceTranslationMap;
		foreach (boost::shared_ptr<Slice> slice, *slicesValue) {
			SliceHash oldHash = slice->hashValue();
			slice->translate(bound.upperLeft());
			sliceTranslationMap[oldHash] = slice->hashValue();
		}

		// Update conflict sets to reflect hash changes due to translation.
		foreach (ConflictSet& conflictSet, *conflictsValue) {
			std::set<SliceHash> newHashes;
			foreach (SliceHash oldHash, conflictSet.getSlices())
				newHashes.insert(sliceTranslationMap.at(oldHash));

			conflictSet.clear();
			foreach (SliceHash newHash, newHashes)
				conflictSet.addSlice(newHash);
		}

		// find all slices that should be completely extracted
		getRequiredSlicesAndConflicts(
				*slicesValue,
				*conflictsValue,
				requestedBlocks,
				requiredSlices,
				requiredConflictSets);

		// expandedBlocks will grow, if required slices are still touching the 
		// boundary of expansionBlocks
		Blocks expandedBlocks = expansionBlocks;
		foreach(boost::shared_ptr<Slice> slice, requiredSlices)
			checkWhole(*slice, expansionBlocks, expandedBlocks);

		LOG_ALL(sliceguarantorlog) << "Extracted in: " << expansionBlocks << ", have to grow to at least: " << expandedBlocks << std::endl; 

		util::rect<unsigned int> expansionBlocksSize = _blockUtils.getBoundingBox(expansionBlocks).project_xy();
		util::rect<unsigned int> expandedBlocksSize  = _blockUtils.getBoundingBox(expandedBlocks).project_xy();

		// if the bounding rectangle of the blocks we extracted slices for has 
		// the same size as the one for the blocks we should extract, we are 
		// done
		slicesComplete = (expansionBlocksSize == expandedBlocksSize);

		if (!slicesComplete) {

			LOG_ALL(sliceguarantorlog) << "Need to expand" << std::endl;
			expansionBlocks.addAll(expandedBlocks);

		} else {

			LOG_ALL(sliceguarantorlog) << "Done for section " << z << std::endl;
		}
	}

	// return the final slices and conflicts
	conflictSets.addAll(requiredConflictSets);
	slices.addAll(requiredSlices);

	return expansionBlocks;
}

void
SliceGuarantor::writeSlicesAndConflicts(
		const Slices&       slices,
		const ConflictSets& conflictSets,
		const Blocks&       requestedBlocks,
		const Blocks&       expansionBlocks) {

	// store all slices of non-required expansion blocks
	foreach (const Block& block, expansionBlocks) {

		// is this a non-required expansion block?
		if (requestedBlocks.contains(block))
			continue;

		// get all the slices for this block
		boost::shared_ptr<Slices> blockSlices = collectSlicesByBlock(slices, block);

		// associate them, but don't set the "I am done with this block"-flag
		_sliceStore->associateSlicesToBlock(*blockSlices, block, false);
	}

	// store all slices of the requested blocks
	foreach (const Block& block, requestedBlocks) {

		// get all the slices for this block
		boost::shared_ptr<Slices> blockSlices = collectSlicesByBlock(slices, block);

		// associate them, and set the "I am done with this block"-flag
		_sliceStore->associateSlicesToBlock(*blockSlices, block);
	}

	// store all conflict sets (of only the requested blocks)
	foreach (const Block& block, requestedBlocks) {

		boost::shared_ptr<Slices>       blockSlices       = collectSlicesByBlock(slices, block);
		boost::shared_ptr<ConflictSets> blockConflictSets = collectConflictsBySlices(conflictSets, *blockSlices);

		_sliceStore->associateConflictSetsToBlock(*blockConflictSets, block);
	}
}

void
SliceGuarantor::getRequiredSlicesAndConflicts(
		const Slices&       slices,
		const ConflictSets& conflictSets,
		const Blocks&       requestedBlocks,
		Slices&             requiredSlices,
		ConflictSets&       requiredConflictSets) {

	requiredSlices.clear();
	requiredConflictSets.clear();

	// build a hash -> slice map
	std::map<SliceHash, boost::shared_ptr<Slice> > hashToSlice;
	foreach (boost::shared_ptr<Slice> slice, slices)
		hashToSlice[slice->hashValue()] = slice;

	// get the 2D bounding box of requestedBlocks
	util::rect<int> requestBound = _blockUtils.getBoundingBox(requestedBlocks).project_xy();

	// every slice that overlaps with requestBound is required
	Slices overlappingSlices;
	foreach (boost::shared_ptr<Slice> slice, slices)
		if (slice->getComponent()->getBoundingBox().intersects(requestBound))
			overlappingSlices.add(slice);

	// every slice that is in conflict with an overlapping slice is required
	Slices conflictSlices;
	foreach (const ConflictSet& conflictSet, conflictSets)
		if (containsAny(conflictSet, overlappingSlices)) {

			// all the slices are required
			foreach (SliceHash sliceHash, conflictSet.getSlices())
				conflictSlices.add(hashToSlice[sliceHash]);

			// the conflict set is required
			requiredConflictSets.add(conflictSet);
		}

	// collect all required slices
	requiredSlices.addAll(overlappingSlices);
	requiredSlices.addAll(conflictSlices);
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
SliceGuarantor::collectConflictsBySlices(const ConflictSets& conflictSets, const Slices& slices) {

	boost::shared_ptr<ConflictSets> sliceConflictSets = boost::make_shared<ConflictSets>();

	foreach (ConflictSet conflictSet, conflictSets)
		if (containsAny(conflictSet, slices))
			sliceConflictSets->add(conflictSet);

	return sliceConflictSets;
}

bool
SliceGuarantor::containsAny(const ConflictSet& conflictSet, const Slices& slices) {

	foreach (const boost::shared_ptr<Slice> slice, slices)
		if (conflictSet.getSlices().count(slice->hashValue()))
			return true;

	return false;
}

void
SliceGuarantor::checkWhole(
		const Slice&  slice,
		const Blocks& expansionBlocks,
		Blocks&       expandedBlocks) const {

	util::rect<unsigned int> sliceBound = slice.getComponent()->getBoundingBox();
	util::rect<unsigned int> blockBound = _blockUtils.getBoundingBox(expansionBlocks).project_xy();
	
	LOG_ALL(sliceguarantorlog)
			<< "block bound: " << blockBound
			<< ", slice bound: " << sliceBound
			<< " for slice " << slice.getId()
			<< std::endl;
	
	if (sliceBound.minX <= blockBound.minX)
	{
		Blocks expandBlocks = Blocks(expansionBlocks);
		_blockUtils.expand(
				expandBlocks,
				0, 0, 0,
				1, 0, 0);
		expandedBlocks.addAll(expandBlocks);
		LOG_ALL(sliceguarantorlog) << "Slice touches -x boundary" << std::endl;
	}
	
	if (sliceBound.maxX >= blockBound.maxX)
	{
		Blocks expandBlocks = Blocks(expansionBlocks);
		_blockUtils.expand(
				expandBlocks,
				1, 0, 0,
				0, 0, 0);
		expandedBlocks.addAll(expandBlocks);
		LOG_ALL(sliceguarantorlog) << "Slice touches +x boundary" << std::endl;
	}

	if (sliceBound.minY <= blockBound.minY)
	{
		Blocks expandBlocks = Blocks(expansionBlocks);
		_blockUtils.expand(
				expandBlocks,
				0, 0, 0,
				0, 1, 0);
		expandedBlocks.addAll(expandBlocks);
		LOG_ALL(sliceguarantorlog) << "Slice touches -y boundary" << std::endl;
	}
	
	if (sliceBound.maxY >= blockBound.maxY)
	{
		Blocks expandBlocks = Blocks(expansionBlocks);
		_blockUtils.expand(
				expandBlocks,
				0, 1, 0,
				0, 0, 0);
		expandedBlocks.addAll(expandBlocks);
		LOG_ALL(sliceguarantorlog) << "Slice touches +y boundary" << std::endl;
	}
}
