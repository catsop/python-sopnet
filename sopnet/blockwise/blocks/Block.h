#ifndef SOPNET_BLOCKWISE_BLOCKS_BLOCK_H__
#define SOPNET_BLOCKWISE_BLOCKS_BLOCK_H__

#include <util/point.hpp>

/**
 * A lightweight data structure representing a block in a stack by its 
 * coordinates.
 */
class Block {

public:

	Block(const util::point<unsigned int, 3>& coordinates) :
		_coordinates(coordinates) {}

	Block(unsigned int x, unsigned int y, unsigned int z) :
		_coordinates(x, y, z) {}

	unsigned int x() const { return _coordinates.x(); }
	unsigned int y() const { return _coordinates.y(); }
	unsigned int z() const { return _coordinates.z(); }

	/**
	 * Provides a total ordering on blocks based on their cooridinates.
	 */
	bool operator<(const Block& other) const {

		return (_coordinates < other._coordinates);
	}

	friend std::ostream& operator<<(std::ostream& os, const Block& b) {
		os << b._coordinates;
		return os;
	}

private:

	util::point<unsigned int, 3> _coordinates;
};

#endif // SOPNET_BLOCKWISE_BLOCKS_BLOCK_H__

