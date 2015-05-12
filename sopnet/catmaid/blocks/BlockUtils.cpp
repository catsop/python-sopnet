#include "BlockUtils.h"

BlockUtils::BlockUtils(const ProjectConfiguration& configuration) :
	_volumeSize(configuration.getVolumeSize()),
	_blockSize(configuration.getBlockSize()),
	_coreSize(configuration.getCoreSize()),
	_coreSizeInVoxels(configuration.getCoreSize()*configuration.getBlockSize()) {

	util::point<unsigned int, 3> numBlocks = (_volumeSize + _blockSize        - util::point<unsigned int, 3>(1, 1, 1))/_blockSize;
	util::point<unsigned int, 3> numCores  = (_volumeSize + _coreSizeInVoxels - util::point<unsigned int, 3>(1, 1, 1))/_coreSizeInVoxels;

	_validBlockCoordinates = util::box<unsigned int, 3>(
			0, 0, 0,
			numBlocks.x(), numBlocks.y(), numBlocks.z());
	_validCoreCoordinates = util::box<unsigned int, 3>(
			0, 0, 0,
			numCores.x(), numCores.y(), numCores.z());
}

void
BlockUtils::expand(
		Blocks& blocks,
		unsigned int posX, unsigned int posY, unsigned int posZ,
		unsigned int negX, unsigned int negY, unsigned int negZ) const {

	// expand in x
	expandOneDimension(blocks, 0, posX, negX);

	// expand in y
	expandOneDimension(blocks, 1, posY, negY);

	// expand in z
	expandOneDimension(blocks, 2, posZ, negZ);
}

util::box<unsigned int, 3>
BlockUtils::getBoundingBox(const Block& block) const {

	return util::box<unsigned int, 3>(
			 block.x()     *_blockSize.x(),
			 block.y()     *_blockSize.y(),
			 block.z()     *_blockSize.z(),
			(block.x() + 1)*_blockSize.x(),
			(block.y() + 1)*_blockSize.y(),
			(block.z() + 1)*_blockSize.z());
}

util::box<unsigned int, 3>
BlockUtils::getBoundingBox(const Blocks& blocks) const {

	util::box<unsigned int, 3> boundingBox(0, 0, 0, 0, 0, 0);

	foreach (const Block& block, blocks) {

		if (boundingBox.volume() == 0)
			boundingBox = getBoundingBox(block);
		else
			boundingBox.fit(getBoundingBox(block));
	}

	return boundingBox;
}

util::box<unsigned int, 3>
BlockUtils::getBoundingBox(const Core& core) const {

	return util::box<unsigned int, 3>(
			 core.x()     *_coreSizeInVoxels.x(),
			 core.y()     *_coreSizeInVoxels.y(),
			 core.z()     *_coreSizeInVoxels.z(),
			(core.x() + 1)*_coreSizeInVoxels.x(),
			(core.y() + 1)*_coreSizeInVoxels.y(),
			(core.z() + 1)*_coreSizeInVoxels.z());
}

Block
BlockUtils::getBlockAtLocation(const util::point<unsigned int, 3>& location) const {

	return Block(location/_blockSize);
}

Blocks
BlockUtils::getBlocksInBox(const util::box<unsigned int, 3>& box) const {

	util::point<unsigned int, 3> minBlockCoordinate = box.min()/_blockSize;
	util::point<unsigned int, 3> numBlocks = (box.min() - minBlockCoordinate*_blockSize
			+ box.size() + _blockSize - util::point<unsigned int, 3>(1, 1, 1))/_blockSize;

	return collectBlocks(minBlockCoordinate, numBlocks);
}

Core
BlockUtils::getCoreAtLocation(const util::point<unsigned int, 3>& location) const {

	return Core(location/_coreSizeInVoxels);
}

Blocks
BlockUtils::getCoreBlocks(const Core& core) const {

	util::point<unsigned int, 3> minBlockCoordinate = core.getCoordinates()*_coreSize;
	util::point<unsigned int, 3> numBlocks = _coreSize;

	return collectBlocks(minBlockCoordinate, numBlocks);
}

Blocks
BlockUtils::getCoresBlocks(const Cores& cores) const {

	Blocks blocks;

	foreach (const Core& core, cores) {

		blocks.addAll(getCoreBlocks(core));
	}

	return blocks;
}

Blocks
BlockUtils::collectBlocks(
	util::point<unsigned int, 3> start,
	util::point<unsigned int, 3> numBlocks) const {

	Blocks blocks;

	for (unsigned int x = start.x(); x < start.x() + numBlocks.x(); x++)
	for (unsigned int y = start.y(); y < start.y() + numBlocks.y(); y++)
	for (unsigned int z = start.z(); z < start.z() + numBlocks.z(); z++)
		if (isValidBlockCoordinate(x, y, z))
			blocks.add(Block(x, y, z));

	return blocks;
}

util::box<unsigned int, 3>
BlockUtils::getVolumeBoundingBox() const {

	return util::box<unsigned int, 3>(
			0, 0, 0,
			_volumeSize.x(), _volumeSize.y(), _volumeSize.z());
}

void
BlockUtils::expandOneDimension(
		Blocks& blocks,
		int dim,
		unsigned int pos,
		unsigned int neg) const {

	Blocks newBlocks;

	foreach (const Block& block, blocks) {

		for (int i = -neg; i <= static_cast<int>(pos); i++) {

			int x = static_cast<int>(block.x());
			int y = static_cast<int>(block.y());
			int z = static_cast<int>(block.z());

			switch (dim) {

				case 0:
					x += i;
					break;
				case 1:
					y += i;
					break;
				default:
					z += i;
			}

			if (isValidBlockCoordinate(x, y, z))
				newBlocks.add(Block(x, y, z));
		}
	}

	blocks.addAll(newBlocks);
}

bool
BlockUtils::isValidBlockCoordinate(int x, int y, int z) const {

	return _validBlockCoordinates.contains(util::point<int, 3>(x, y, z));
}
