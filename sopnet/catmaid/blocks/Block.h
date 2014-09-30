#ifndef SOPNET_CATMAID_BLOCKS_BLOCK_H__
#define SOPNET_CATMAID_BLOCKS_BLOCK_H__

#include <util/point3.hpp>

/**
 * A lightweight data structure representing a block in a stack by its 
 * coordinates.
 */
class Block {

public:

	Block(const util::point3<unsigned int>& coordinates) :
		_coordinates(coordinates) {}

	Block(unsigned int x, unsigned int y, unsigned int z) :
		_coordinates(x, y, z) {}

	unsigned int x() const { return _coordinates.x; }
	unsigned int y() const { return _coordinates.y; }
	unsigned int z() const { return _coordinates.z; }

	/**
	 * Provides a total ordering on blocks based on their cooridinates.
	 */
	bool operator<(const Block& other) const {

		return (_coordinates < other._coordinates);
	}

private:

	util::point3<unsigned int> _coordinates;
};

#endif // SOPNET_CATMAID_BLOCKS_BLOCK_H__

