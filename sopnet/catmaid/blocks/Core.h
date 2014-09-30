#ifndef SOPNET_CATMAID_BLOCKS_CORE_H__
#define SOPNET_CATMAID_BLOCKS_CORE_H__

#include <util/point3.hpp>

/**
 * A lightweight data structure representing a core in a stack by its 
 * coordinates.
 */
class Core {

public:

	Core(const util::point3<unsigned int>& coordinates) :
		_coordinates(coordinates) {}

	Core(unsigned int x, unsigned int y, unsigned int z) :
		_coordinates(x, y, z) {}

	unsigned int x() const { return _coordinates.x; }
	unsigned int y() const { return _coordinates.y; }
	unsigned int z() const { return _coordinates.z; }

	/**
	 * Get a point3 that represents the coordinates of this core.
	 */
	const util::point3<unsigned int>& getCoordinates() const { return _coordinates; }

	/**
	 * Provides a total ordering on cores based on their cooridinates.
	 */
	bool operator<(const Core& other) const {

		return _coordinates < other._coordinates;
	}

private:

	util::point3<unsigned int> _coordinates;
};

#endif // SOPNET_CATMAID_BLOCKS_CORE_H__


