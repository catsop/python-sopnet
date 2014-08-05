#ifndef SLICE_GUARANTOR_H__
#define SLICE_GUARANTOR_H__

#include <set>
#include <boost/shared_ptr.hpp>

#include <catmaid/persistence/SliceStore.h>
#include <catmaid/persistence/StackStore.h>
#include <sopnet/sopnet/block/Box.h>
#include <sopnet/sopnet/block/Blocks.h>
#include <pipeline/all.h>
#include <pipeline/Value.h>
#include <sopnet/slices/ConflictSets.h>
#include <imageprocessing/io/ImageBlockStackReader.h>
#include <imageprocessing/io/ImageBlockFactory.h>
#include <imageprocessing/MserParameters.h>
#include "SliceGuarantorParameters.h"

/**
 * SliceGuarantor is a class that, given a sub-stack (measured in Blocks), guarantees that all
 * Slices in that substack will exist in the given SliceStore after guaranteeSlices is called.
 * If this guarantee cannot be made, say because the membrane images are not yet available,
 * guaranteeSlices will report those Blocks and exit (in this case, no guarantee about the slices
 * has been made).
 * 
 * This extends to any conflict sets over the slices. In the MSER sense, two Slices conflict if
 * one is the ancestor of the other. A SliceGuarantor will guarantee that all Slices that belong to
 * the same conflict set as a Slice in the guaranteed substack are also populated in the Slice Store,
 * even if they don't overlap with the guaranteed substack.
 * 
 * 
 */

class SliceGuarantor : public pipeline::SimpleProcessNode<>
{
public:
	/**
	 * Extracts the slices for the 
	 * requested blocks. Returns empty Blocks for success. If extraction
	 * was not possible, Blocks in the output will indicate those for
	 * which it failed. Typically, this means that not all images were available in
	 * the given Blocks.
	 */
	Blocks guaranteeSlices(const Blocks& blocks);
	
	void setMserParameters(const boost::shared_ptr<MserParameters> mserParameters);
	
	void setSliceStore(const boost::shared_ptr<SliceStore> sliceStore);
	
	void setMaximumArea(const unsigned int area);
	
	void setStackStore(const boost::shared_ptr<StackStore> stackStore);

private:
	bool checkSlices();
	
	bool sizeOk(util::point3<unsigned int> size);
	
	void collectOutputSlices(pipeline::Value<Slices>& extractedSlices,
							pipeline::Value<ConflictSets>& extractedConflict,
							const boost::shared_ptr<Slices>& slices);
	
	bool extractSlices(const unsigned int z,
							  const boost::shared_ptr<Slices> slices,
							  const boost::shared_ptr<ConflictSets> conflictSets,
							  const boost::shared_ptr<Blocks> extractBlocks);
	
	bool containsAny(const ConflictSet& conflictSet, const std::set<unsigned int>& idSet);
	
	/**
	 * Helper function that checks whether a Slice can be considered whole or
	 * not
	 */
	void checkWhole(const boost::shared_ptr<Slice>& slice,
					const boost::shared_ptr<Blocks>& extractBlocks,
					const boost::shared_ptr<Blocks>& nbdBlocks) const;
	
	boost::shared_ptr<MserParameters> _mserParameters;
	boost::shared_ptr<SliceStore> _sliceStore;
	boost::shared_ptr<unsigned int> _maximumArea;
	boost::shared_ptr<StackStore> _stackStore;
};

#endif //SLICE_GUARANTOR_H__
