#ifndef SOPNET_BLOCKWISE_BLOCKS_CORES_H__
#define SOPNET_BLOCKWISE_BLOCKS_CORES_H__

#include <set>
#include <util/foreach.h>
#include "Core.h"

class Cores {

	typedef std::set<Core> blocks_type;

public:

	typedef blocks_type::iterator       iterator;
	typedef blocks_type::const_iterator const_iterator;

	/**
	 * Add a single block to this collection.
	 */
	void add(const Core& block) { _cores.insert(block); }

	/**
	 * Add a set of blocks to this collection.
	 */
	void addAll(const Cores& blocks) { for (const Core& block : blocks) add(block); }

	/**
	 * Check whether this collection is empty.
	 */
	bool empty() { return _cores.empty(); }

	/**
	 * Get the number of cores in this collection.
	 */
	unsigned int size() { return _cores.size(); }

	/**
	 * Iterator access to the blocks.
	 */
	iterator begin() { return _cores.begin(); }
	iterator end() { return _cores.end(); }

	/**
	 * Const iterator access to the blocks.
	 */
	const_iterator begin() const { return _cores.begin(); }
	const_iterator end() const { return _cores.end(); }

private:

	blocks_type _cores;
};

#endif // SOPNET_BLOCKWISE_BLOCKS_CORES_H__
