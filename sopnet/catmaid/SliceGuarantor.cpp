#include "SliceGuarantor.h"

#include <map>
#include <set>
#include <imageprocessing/ImageExtractor.h>
#include <sopnet/sopnet/slices/SliceExtractor.h>
#include <sopnet/sopnet/slices/Slice.h>
#include <util/rect.hpp>
#include <util/Logger.h>
#include <util/foreach.h>
#include <pipeline/Value.h>

logger::LogChannel sliceguarantorlog("sliceguarantorlog", "[SliceGuarantor] ");

using std::vector;
using boost::shared_ptr;
using boost::make_shared;
using std::map;
using std::set;

SliceGuarantor::SliceGuarantor()
{
	registerInput(_blocks, "blocks");
	registerInput(_sliceStore, "slice store");
	registerInput(_stackStore, "stack store");
	registerInput(_maximumArea, "maximum area", pipeline::Optional);
	registerInput(_mserParameters, "mser parameters", pipeline::Optional);

	registerOutput(_needBlocks, "image blocks");
}

void
SliceGuarantor::updateOutputs()
{
	*_needBlocks = *guaranteeSlices();
}

/**
 * Checks whether slices have already been extracted for our Block request.
 */
bool
SliceGuarantor::checkSlices()
{
	bool slicesExtracted = true;

	// Check to see whether each block in guaranteeBlocks has already had its slices extracted.
	// If this is the case, we have no work to do
	foreach (boost::shared_ptr<Block> block, *_blocks)
	{
		slicesExtracted &= block->getSlicesFlag();
	}
	
	return slicesExtracted;
	
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


pipeline::Value<Blocks>
SliceGuarantor::guaranteeSlices()
{
	pipeline::Value<Blocks> extractBlocks = pipeline::Value<Blocks>();
	pipeline::Value<Slices> slices = pipeline::Value<Slices>();
	pipeline::Value<ConflictSets> conflictSets = pipeline::Value<ConflictSets>();
	boost::shared_ptr<SliceWriter> sliceWriter = boost::make_shared<SliceWriter>();
	
	updateInputs();
	
	if (checkSlices())
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
	vector<shared_ptr<Slices> > slicesVector(_blocks->size().z);
	// extracted conflict sets
	vector<shared_ptr<ConflictSets> > conflictSetsVector(_blocks->size().z); 
	
	// This isn't *really* true.
	bool allBad = true;

	// Extract slices independently by z.
	for (unsigned int i = 0; i < _blocks->size().z; ++i)
	{
		unsigned int z = i + _blocks->location().z;
		shared_ptr<Slices> zSlices = make_shared<Slices>();
		shared_ptr<ConflictSets> zConflict = make_shared<ConflictSets>();
		shared_ptr<Blocks> zBlocks = make_shared<Blocks>();
		
		allBad = !extractSlices(z, zSlices, zConflict, zBlocks) && allBad;
		
		slicesVector[i] = zSlices;
		conflictSetsVector[i] = zConflict;
		
		extractBlocks->addAll(zBlocks);
	}

	// If all sections yielded empty images, then we need to regenerate the image stack.
	if (allBad)
	{
		return extractBlocks;
	}
	
	for (unsigned int i = 0; i < _blocks->size().z; ++i)
	{
		slices->addAll(*slicesVector[i]);
		conflictSets->addAll(*conflictSetsVector[i]);
	}
	
	sliceWriter->setInput("blocks", extractBlocks);
	sliceWriter->setInput("slices", slices);
	sliceWriter->setInput("conflict sets", conflictSets);
	sliceWriter->setInput("store", _sliceStore);
	
	sliceWriter->writeSlices();
	
	return pipeline::Value<Blocks>();
}

bool
SliceGuarantor::extractSlices(const unsigned int z,
							  const shared_ptr<Slices> slices,
							  const shared_ptr<ConflictSets> conflictSets,
							  const shared_ptr<Blocks> extractBlocks)
{
	LOG_ALL(sliceguarantorlog) << "Setting up mini pipeline" << std::endl;
	shared_ptr<Blocks> nbdBlocks;	
	
	bool okSlices = false;
	shared_ptr<SliceExtractor<unsigned char> > sliceExtractor =
		make_shared<SliceExtractor<unsigned char> >(z);

	pipeline::Value<Slices> slicesValue;
	pipeline::Value<ConflictSets> conflictValue;
	
	extractBlocks->addAll(_blocks);
	// Dilate once beforehand.
	extractBlocks->dilateXY();

	sliceExtractor->setInput("force explanation", pipeline::Value<bool>(true));

	while (!okSlices && sizeOk(extractBlocks->size()))
	{
		util::rect<unsigned int> bound = *extractBlocks;
		util::point<int> translate(extractBlocks->location().x, extractBlocks->location().y);
		Box<> box(bound, z, 1);
		shared_ptr<Image> image = (*_stackStore->getImageStack(box))[0];
		
		if (image->width() * image->height() == 0)
		{
			return false;
		}
		
		nbdBlocks = make_shared<Blocks>(_blocks);

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
			if (_blocks->overlaps(slice->getComponent()))
			{
				checkWhole(slice, extractBlocks, nbdBlocks);
			}
		}
		
		if (!(okSlices = extractBlocks->size() == nbdBlocks->size()))
		{
			extractBlocks->addAll(nbdBlocks);
		}
	}
	
	conflictSets->addAll(*conflictValue);
	collectOutputSlices(slicesValue, conflictValue, slices);
	
	return true;
}

/**
 * Collect all of the Slice's that we need to store from extractedSlices and put
 * them into slices.
 */
void
SliceGuarantor::collectOutputSlices(pipeline::Value<Slices>& extractedSlices,
								   pipeline::Value<ConflictSets>& extractedConflict,
								   const shared_ptr<Slices>& slices)
{
	map<unsigned int, boost::shared_ptr<Slice> > idSliceMap;
	set<unsigned int> idSet;
	// Must be separate from idSet to avoid problems with the possibility of multiply-overlapping
	// conflict sets.
	set<unsigned int> conflictCliqueIdSet;
	
	// Collection in two steps:
	// 1) Find all Slice's that overlap the guarantee blocks
	// 2) Find all Slice's that are in a conflict set with any of the Slice's found in (1)
	
	foreach (boost::shared_ptr<Slice> slice, *extractedSlices)
	{
		// Map slice id to slice object
		idSliceMap[slice->getId()] = slice;
		
		// If the slice overlaps the guarantee blocks, add it to the set
		if (_blocks->overlaps(slice->getComponent()))
		{
			idSet.insert(slice->getId());
		}
	}
	
	foreach (const ConflictSet conflictSet, *extractedConflict)
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
		slices->add(idSliceMap[id]);
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
SliceGuarantor::checkWhole(const boost::shared_ptr<Slice>& slice,
						   const boost::shared_ptr<Blocks>& extractBlocks,
						   const boost::shared_ptr<Blocks>& nbdBlocks) const
{
	util::rect<unsigned int> sliceBound = slice->getComponent()->getBoundingBox();
	
	point3<unsigned int> blockLocation = extractBlocks->location();
	point3<unsigned int> blockSize = extractBlocks->size();
	
	if (sliceBound.minX <= blockLocation.x)
	{
		Blocks expandBlocks = Blocks(*extractBlocks);
		expandBlocks.expand(util::point3<int>(-1, 0, 0));
		nbdBlocks->addAll(expandBlocks.getBlocks());
	}
	else if (sliceBound.maxX >= blockLocation.x + blockSize.x - 1)
	{
		Blocks expandBlocks = Blocks(*extractBlocks);
		expandBlocks.expand(util::point3<int>(1, 0, 0));
		nbdBlocks->addAll(expandBlocks.getBlocks());
	}
	
	if (sliceBound.minY <= blockLocation.y)
	{
		Blocks expandBlocks = Blocks(*extractBlocks);
		expandBlocks.expand(util::point3<int>(0, -1, 0));
		nbdBlocks->addAll(expandBlocks.getBlocks());
	}
	else if (sliceBound.maxY >= blockLocation.y + blockSize.y - 1)
	{
		Blocks expandBlocks = Blocks(*extractBlocks);
		expandBlocks.expand(util::point3<int>(0, 1, 0));
		nbdBlocks->addAll(expandBlocks.getBlocks());
	}
}

