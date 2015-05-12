#ifndef HOST_TUBES_TUBE_PROPERTY_MAP_H__
#define HOST_TUBES_TUBE_PROPERTY_MAP_H__

#include <map>
#include "TubeId.h"

template <typename T>
class TubePropertyMap {

	typedef std::map<TubeId, T> map_type;

public:

	typedef typename map_type::iterator       iterator;
	typedef typename map_type::const_iterator const_iterator;

	T& operator[](TubeId id) { return _map[id]; }
	const T& operator[](TubeId id) const { return _map.at(id); }

	/**
	 * Insert an element into the map. Returns true, if a new element was 
	 * inserted, and false, if an existing element was overwritten.
	 */
	bool insert(TubeId id, T&& value) { return _map.insert(std::make_pair(id, std::forward<T>(value))).second; }

	/**
	 * Check if an element for the given id exists.
	 */
	bool contains(TubeId id) const { return _map.count(id); }

	/**
	 * Remove the element with the given id. Returns true, if the element was 
	 * contained.
	 */
	bool remove(TubeId id) { return _map.erase(id); }

	/**
	 * Direct iterator access.
	 */
	iterator       begin()       { return _map.begin(); }
	const_iterator begin() const { return _map.begin(); }
	iterator       end()       { return _map.end(); }
	const_iterator end() const { return _map.end(); }

	/**
	 * Clear the property map.
	 */
	void clear() { _map.clear(); }

	std::size_t size() const { return _map.size(); }

private:

	map_type _map;
};

#endif // HOST_TUBES_TUBE_PROPERTY_MAP_H__

