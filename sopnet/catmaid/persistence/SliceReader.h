#ifndef SLICE_READER_H__
#define SLICE_READER_H__

#include <deque>
#include <boost/unordered_set.hpp>
#include <imageprocessing/ComponentTrees.h>
#include <catmaid/persistence/SliceStore.h>
#include <sopnet/block/BlockManager.h>
#include <sopnet/block/Box.h>
#include <sopnet/slices/Slices.h>
#include <pipeline/all.h>

class SliceReader : public pipeline::SimpleProcessNode<>
{
public:
	SliceReader();

private:
	void updateOutputs();
	
	void addUnique(const boost::shared_ptr<Slices>& inSlices,
				   const boost::shared_ptr<Slices>& recvSlices,
				   boost::unordered_set<Slice>& set);

	pipeline::Input<Blocks> _blocks;
	pipeline::Input<BlockManager> _blockManager;
	pipeline::Input<SliceStore> _store;
	pipeline::Output<Slices> _slices;
	pipeline::Output<ConflictSets> _conflictSets;
	
	bool _sourceIsBox;
};

#endif //SLICE_READER_H__
