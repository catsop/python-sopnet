#ifndef SOPNET_BLOCKWISE_BLOCKS_BLOCK_UTILS_H__
#define SOPNET_BLOCKWISE_BLOCKS_BLOCK_UTILS_H__

#include <blockwise/ProjectConfiguration.h>
#include <util/box.hpp>
#include "Block.h"
#include "Blocks.h"
#include "Core.h"
#include "Cores.h"

class BlockUtils {

public:

	BlockUtils(const ProjectConfiguration& configuration);

	/**
	 * Expand a set of blocks in the given directions. For each block in blocks, 
	 * the next posX blocks in the positive x direction and the next negX blocks 
	 * in the negative x direction are added. This is repeated for the y and z 
	 * dimension as well. Only blocks that are within the bounds of the stack 
	 * will be added.
	 *
	 * @param blocks
	 *              The blocks to expand. Will be modified.
	 *
	 * @param posX, posY, posZ
	 *              The amount by wich to expand in the positive x, y, and z 
	 *              direction.
	 *
	 * @param negX, negY, negZ
	 *              The amount by wich to expand in the negative x, y, and z 
	 *              direction.
	 */
	void expand(
			Blocks& blocks,
			unsigned int posX, unsigned int posY, unsigned int posZ,
			unsigned int negX, unsigned int negY, unsigned int negZ) const;

	/**
	 * Get the bounding box of a single block in pixels.
	 */
	util::box<unsigned int, 3> getBoundingBox(const Block& block) const;

	/**
	 * Get the bounding box of a set of blocks in pixels.
	 */
	util::box<unsigned int, 3> getBoundingBox(const Blocks& blocks) const;

	/**
	 * Get the bounding box of a single core in pixels.
	 */
	util::box<unsigned int, 3> getBoundingBox(const Core& core) const;

	/**
	 * Get the block that contains the given pixel location.
	 */
	Block getBlockAtLocation(const util::point<unsigned int, 3>& location) const;

	/**
	 * Get all blocks that intersect the given bounding box.
	 */
	Blocks getBlocksInBox(const util::box<unsigned int, 3>& box) const;

	/**
	 * Get the core that contains the given pixel location.
	 */
	Core getCoreAtLocation(const util::point<unsigned int, 3>& location) const;

	/**
	 * Get all the blocks that constitute the gicen core.
	 */
	Blocks getCoreBlocks(const Core& core) const;

	/**
	 * Get all the blocks of all the given cores
	 */
	Blocks getCoresBlocks(const Cores& cores) const;

	/**
	 * Get the bounding box of the whole volume in pixels.
	 */
	util::box<unsigned int, 3> getVolumeBoundingBox() const;

private:

	// get a set of blocks by specifying a start coordinate and the number of 
	// blocks in each dimension
	Blocks collectBlocks(
			util::point<unsigned int, 3> start,
			util::point<unsigned int, 3> numBlocks) const;

	// expand the blocks in one dimension by the given number of blocks in the 
	// positive and negative direction
	void expandOneDimension(
			Blocks& blocks,
			int dim,
			unsigned int pos,
			unsigned int neg) const;

	// check whether the given coordinates correspond to a block in the stack
	bool isValidBlockCoordinate(int x, int y, int z) const;

	// the size of the whole volume
	const util::point<unsigned int, 3>& _volumeSize;

	// block size in voxels
	const util::point<unsigned int, 3>& _blockSize;

	// core size in blocks and voxels
	const util::point<unsigned int, 3>& _coreSize;
	const util::point<unsigned int, 3> _coreSizeInVoxels;

	// boxes with all the valid block and core coordinates
	util::box<unsigned int, 3> _validBlockCoordinates;
	util::box<unsigned int, 3> _validCoreCoordinates;
};

#endif // SOPNET_BLOCKWISE_BLOCKS_BLOCK_UTILS_H__

