#ifndef SLICE_WRITER_H__
#define SLICE_WRITER_H__

#include <pipeline/all.h>
#include <sopnet/block/Blocks.h>
#include <sopnet/slices/Slices.h>
#include <sopnet/slices/ConflictSets.h>
#include <catmaid/persistence/SliceStore.h>

class SliceWriter : public pipeline::SimpleProcessNode<>
{
public:
	/**
	 * Construct a SliceWriter, which writes Slices to a given Block in a given SliceStore.
	 * Inputs:
	 *   Blocks       - "blocks"
	 *   Slices       - "slices"
	 *   SliceStore   - "store"
	 *   ConflictSets - "conflict sets"
	 */
	SliceWriter();
	
	void writeSlices();
	
private:
	
	void updateOutputs(){}
	
	bool containsAny(ConflictSet& conflictSet,
					 pipeline::Value<Slices>& slices);
	
	pipeline::Value<Slices> collectSlicesByBlocks(const boost::shared_ptr<Block> block);
	pipeline::Value<ConflictSets> collectConflictBySlices(pipeline::Value<Slices> slices);

	pipeline::Input<Blocks> _blocks;
	pipeline::Input<Slices> _slices;
	pipeline::Input<SliceStore> _store;
	pipeline::Input<ConflictSets> _conflictSets;
};


#endif //SLICE_WRITER_H__
