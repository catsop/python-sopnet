#ifndef SLICE_READER_H__
#define SLICE_READER_H__

#include <deque>
#include <boost/unordered_set.hpp>
#include <imageprocessing/ComponentTrees.h>
#include <catmaid/persistence/SliceStore.h>
#include <sopnet/block/CoreManager.h>
#include <sopnet/block/Box.h>
#include <sopnet/slices/Slices.h>
#include <pipeline/all.h>

class SliceReader : public pipeline::SimpleProcessNode<>
{
public:
	/**
	 * Construct a SliceReader, which reads Slices and ConflictSets from a SliceStore.
	 * Inputs:
	 *   Blocks     - "blocks"
	 *   SliceStore - "store"
	 * Outputs:
	 *   Slices       - "slices"
	 *   ConflictSets - "conflict sets"
	 */
	SliceReader();

private:
	void updateOutputs();
	
	static bool slicePtrComparator(const boost::shared_ptr<Slice> slice1,
								   const boost::shared_ptr<Slice> slice2);
	
	pipeline::Input<Blocks> _blocks;
	pipeline::Input<SliceStore> _store;
	pipeline::Output<Slices> _slices;
	pipeline::Output<ConflictSets> _conflictSets;
};

#endif //SLICE_READER_H__
