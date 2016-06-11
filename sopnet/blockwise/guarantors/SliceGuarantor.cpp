#include "SliceGuarantor.h"

#include <map>
#include <set>
#include <unordered_set>
#include <imageprocessing/ImageExtractor.h>
#include <slices/SliceExtractor.h>
#include <slices/Slice.h>
#include <util/box.hpp>
#include <util/Logger.h>
#include <pipeline/Process.h>
#include <pipeline/Value.h>

logger::LogChannel sliceguarantorlog("sliceguarantorlog", "[SliceGuarantor] ");

SliceGuarantor::SliceGuarantor(
			const ProjectConfiguration&   projectConfiguration,
			boost::shared_ptr<SliceStore> sliceStore,
			boost::shared_ptr<StackStore<IntensityImage> > stackStore) :
	_sliceStore(sliceStore),
	_stackStore(stackStore),
	_blockUtils(projectConfiguration) {}

void
SliceGuarantor::setComponentTreeExtractorParameters(
		const boost::shared_ptr<ComponentTreeExtractorParameters<IntensityImage::value_type> > parameters) {

	_parameters = parameters;
}

Blocks
SliceGuarantor::guaranteeSlices(const Blocks& requestedBlocks) {

	// have the slices been extracted already?
	if (checkSlices(requestedBlocks))
		return Blocks();


	// get the number of sections
	util::box<unsigned int, 3> blockBoundingBox = _blockUtils.getBoundingBox(requestedBlocks);
	util::box<unsigned int, 3> volumeBoundingBox = _blockUtils.getVolumeBoundingBox();
	unsigned int firstSection = blockBoundingBox.min().z();
	// Clamp the number of sections within the volume extents.
	unsigned int numSections  =
		std::min(blockBoundingBox.depth(), volumeBoundingBox.max().z() - firstSection);

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
	for (const Block& block : blocks) {
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
		util::box<unsigned int, 2> bound = _blockUtils.getBoundingBox(expansionBlocks).project<2>();

		// box for only the current section
		util::box<unsigned int, 3> sectionBox(bound.min().x(), bound.min().y(), z, bound.max().x(), bound.max().y(), z + 1);

		// get the image for this box
		boost::shared_ptr<IntensityImage> image = (*_stackStore->getImageStack(sectionBox))[0];

		LOG_ALL(sliceguarantorlog) << "Processing over " << bound << std::endl;

		// extract slices and conflict sets
		sliceExtractor->setInput("membrane", image);
		if (_parameters)
			sliceExtractor->setInput("parameters", _parameters);

		slicesValue = sliceExtractor->getOutput("slices");
		conflictsValue = sliceExtractor->getOutput("conflict sets");
		
		LOG_DEBUG(sliceguarantorlog) << "Extracted " << slicesValue->size() << " slices" << std::endl;

		LOG_ALL(sliceguarantorlog) << "Translating slices to request coordinates" << std::endl;

		// Slices are extracted in [0 0 w h]. Translate them to section 
		// coordinates.
		std::map<SliceHash, SliceHash> sliceTranslationMap;
		for (boost::shared_ptr<Slice> slice : *slicesValue) {
			SliceHash oldHash = slice->hashValue();
			slice->translate(bound.min());
			sliceTranslationMap[oldHash] = slice->hashValue();
		}

		// Update conflict sets to reflect hash changes due to translation.
		for (ConflictSet& conflictSet : *conflictsValue) {
			std::set<SliceHash> newHashes;
			for (SliceHash oldHash : conflictSet.getSlices())
				newHashes.insert(sliceTranslationMap.at(oldHash));

			conflictSet.clear();
			for (SliceHash newHash : newHashes)
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
		for (boost::shared_ptr<Slice> slice : requiredSlices)
			checkWhole(*slice, expandedBlocks);

		LOG_ALL(sliceguarantorlog) << "Extracted in: " << expansionBlocks << ", have to grow to at least: " << expandedBlocks << std::endl; 

		util::box<unsigned int, 2> expansionBlocksSize = _blockUtils.getBoundingBox(expansionBlocks).project<2>();
		util::box<unsigned int, 2> expandedBlocksSize  = _blockUtils.getBoundingBox(expandedBlocks).project<2>();

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
	for (const Block& block : expansionBlocks) {

		// is this a non-required expansion block?
		if (requestedBlocks.contains(block))
			continue;

		// get all the slices for this block
		boost::shared_ptr<Slices> blockSlices = collectSlicesByBlock(slices, block);

		// associate them, but don't set the "I am done with this block"-flag
		_sliceStore->associateSlicesToBlock(*blockSlices, block, false);
	}

	// store all slices of the requested blocks
	for (const Block& block : requestedBlocks) {

		// get all the slices for this block
		boost::shared_ptr<Slices> blockSlices = collectSlicesByBlock(slices, block);

		// associate them, and set the "I am done with this block"-flag
		_sliceStore->associateSlicesToBlock(*blockSlices, block);
	}

	// store all conflict sets (of only the requested blocks)
	for (const Block& block : requestedBlocks) {

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
	for (boost::shared_ptr<Slice> slice : slices)
		hashToSlice[slice->hashValue()] = slice;

	// get the 2D bounding box of requestedBlocks
	util::box<int, 2> requestBound = _blockUtils.getBoundingBox(requestedBlocks).project<2>();

	// every slice that overlaps with requestBound is required
	Slices overlappingSlices;
	for (boost::shared_ptr<Slice> slice : slices)
		if (slice->getComponent()->getBoundingBox().intersects(requestBound))
			overlappingSlices.add(slice);

	// every slice that is in conflict with an overlapping slice is required
	Slices conflictSlices;
	for (const ConflictSet& conflictSet : conflictSets)
		if (containsAny(conflictSet, overlappingSlices)) {

			// all the slices are required
			for (SliceHash sliceHash : conflictSet.getSlices())
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

	util::box<unsigned int, 2> blockRect = _blockUtils.getBoundingBox(block).project<2>();

	boost::shared_ptr<Slices> blockSlices = boost::make_shared<Slices>();

	for (boost::shared_ptr<Slice> slice : slices) {

		util::box<unsigned int, 2> sliceBoundingBox = slice->getComponent()->getBoundingBox();

		if (blockRect.intersects(sliceBoundingBox))
			blockSlices->add(slice);
	}

	return blockSlices;
}

boost::shared_ptr<ConflictSets>
SliceGuarantor::collectConflictsBySlices(const ConflictSets& conflictSets, const Slices& slices) {

	boost::shared_ptr<ConflictSets> sliceConflictSets = boost::make_shared<ConflictSets>();

	std::unordered_set<SliceHash> sliceHashes;
	sliceHashes.reserve(slices.size());
	for (const boost::shared_ptr<Slice> slice : slices)
			sliceHashes.insert(slice->hashValue());

	for (ConflictSet conflictSet : conflictSets) {
			for (const SliceHash slice : conflictSet.getSlices()) {
					if (sliceHashes.count(slice)) {
							sliceConflictSets->add(conflictSet);
							break;
					}
			}
	}

	return sliceConflictSets;
}

bool
SliceGuarantor::containsAny(const ConflictSet& conflictSet, const Slices& slices) {

	for (const boost::shared_ptr<Slice> slice : slices)
		if (conflictSet.getSlices().count(slice->hashValue()))
			return true;

	return false;
}

void
SliceGuarantor::checkWhole(
		const Slice&  slice,
		Blocks&       expandedBlocks) const {

	util::box<unsigned int, 2> sliceBound = slice.getComponent()->getBoundingBox();
	util::box<unsigned int, 2> blockBound = _blockUtils.getBoundingBox(expandedBlocks).project<2>();
	
	LOG_ALL(sliceguarantorlog)
			<< "block bound: " << blockBound
			<< ", slice bound: " << sliceBound
			<< " for slice " << slice.getId()
			<< std::endl;
	
	if (sliceBound.min().x() <= blockBound.min().x())
	{
		_blockUtils.expand(
				expandedBlocks,
				0, 0, 0,
				1, 0, 0);
		LOG_ALL(sliceguarantorlog) << "Slice touches -x boundary" << std::endl;
	}
	
	if (sliceBound.max().x() >= blockBound.max().x())
	{
		_blockUtils.expand(
				expandedBlocks,
				1, 0, 0,
				0, 0, 0);
		LOG_ALL(sliceguarantorlog) << "Slice touches +x boundary" << std::endl;
	}

	if (sliceBound.min().y() <= blockBound.min().y())
	{
		_blockUtils.expand(
				expandedBlocks,
				0, 0, 0,
				0, 1, 0);
		LOG_ALL(sliceguarantorlog) << "Slice touches -y boundary" << std::endl;
	}
	
	if (sliceBound.max().y() >= blockBound.max().y())
	{
		_blockUtils.expand(
				expandedBlocks,
				0, 1, 0,
				0, 0, 0);
		LOG_ALL(sliceguarantorlog) << "Slice touches +y boundary" << std::endl;
	}
}
