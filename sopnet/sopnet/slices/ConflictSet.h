#ifndef SOPNET_SLICES_CONFLICT_SET_H__
#define SOPNET_SLICES_CONFLICT_SET_H__

#include <util/foreach.h>
#include <boost/functional/hash.hpp>

#include <set>

/**
 * Collection of slice ids that are in conflict, i.e., only one of them can be 
 * chosen at the same time.
 */
class ConflictSet {

public:

	void addSlice(unsigned int sliceId) {

		_sliceIds.insert(sliceId);
	}

	void removeSlice(unsigned int sliceId) {

		std::set<unsigned int>::iterator i = _sliceIds.find(sliceId);

		if (i != _sliceIds.end())
			_sliceIds.erase(i);
	}

	void clear() {

		_sliceIds.clear();
	}

	const std::set<unsigned int>& getSlices() const {

		return _sliceIds;
	}
	
	bool operator==(const ConflictSet& other) const{
		
		if (_sliceIds.size() != other._sliceIds.size())
		{
			return false;
		}
		
		// Two ConflictSet's are equal if they contain each other.
		foreach (const unsigned int id, _sliceIds)
		{
			if (!other._sliceIds.count(id))
			{
				return false;
			}
		}
		
		foreach (const unsigned int id, other._sliceIds)
		{
			if (!_sliceIds.count(id))
			{
				return false;
			}
		}
		
		return true;
	}

private:

	std::set<unsigned int> _sliceIds;
};

std::size_t hash_value(const ConflictSet& conflictSet);

#endif // SOPNET_SLICES_CONFLICT_SET_H__

