#ifndef SOPNET_CATMAID_BLOCKS_BLOCKS_H__
#define SOPNET_CATMAID_BLOCKS_BLOCKS_H__

#include <set>
#include <util/foreach.h>
#include "Block.h"

class Blocks {

	typedef std::set<Block> blocks_type;

public:

	typedef blocks_type::iterator       iterator;
	typedef blocks_type::const_iterator const_iterator;

	/**
	 * Add a single block to this collection.
	 */
	void add(const Block& block) { _blocks.insert(block); }

	/**
	 * Add a set of blocks to this collection.
	 */
	void addAll(const Blocks& blocks) { foreach (const Block& block, blocks) add(block); }

	/**
	 * Check whether this collection is empty.
	 */
	bool empty() { return _blocks.empty(); }

	/**
	 * Get the number of blocks in this collection.
	 */
	unsigned int size() { return _blocks.size(); }

	/**
	 * Check whether this collection contains the given block.
	 */
	bool contains(const Block& block) const { return _blocks.count(block); }

	/**
	 * Iterator access to the blocks.
	 */
	iterator begin() { return _blocks.begin(); }
	iterator end() { return _blocks.end(); }

	/**
	 * Const iterator access to the blocks.
	 */
	const_iterator begin() const { return _blocks.begin(); }
	const_iterator end() const { return _blocks.end(); }

private:

	blocks_type _blocks;
};

std::ostream&
operator<<(std::ostream& out, const Blocks& blocks);

#endif // SOPNET_CATMAID_BLOCKS_BLOCKS_H__

