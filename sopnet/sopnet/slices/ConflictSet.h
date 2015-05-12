#ifndef SOPNET_SLICES_CONFLICT_SET_H__
#define SOPNET_SLICES_CONFLICT_SET_H__

#include <util/foreach.h>
#include <boost/functional/hash.hpp>
#include <sopnet/slices/SliceHash.h>

#include <set>

/**
 * Collection of slice ids that are in conflict, i.e., only one of them can be 
 * chosen at the same time.
 */
class ConflictSet {

public:

	void addSlice(SliceHash sliceId) {

		_sliceIds.insert(sliceId);
	}

	void removeSlice(SliceHash sliceId) {

		std::set<SliceHash>::iterator i = _sliceIds.find(sliceId);

		if (i != _sliceIds.end())
			_sliceIds.erase(i);
	}

	void clear() {

		_sliceIds.clear();
	}

	const std::set<SliceHash>& getSlices() const {

		return _sliceIds;
	}

	bool isMaximalClique() const {

		return _isMaximalClique;
	}

	void setMaximalClique(bool clique) {

		_isMaximalClique = clique;
	}
	
	bool operator==(const ConflictSet& other) const{
		
		if (_sliceIds.size() != other._sliceIds.size())
		{
			return false;
		}
		
		// Two ConflictSet's are equal if they contain each other.
		for (const SliceHash id : _sliceIds)
		{
			if (!other._sliceIds.count(id))
			{
				return false;
			}
		}
		
		for (const SliceHash id : other._sliceIds)
		{
			if (!_sliceIds.count(id))
			{
				return false;
			}
		}
		
		return true;
	}

private:

	std::set<SliceHash> _sliceIds;

	bool _isMaximalClique;
};

std::size_t hash_value(const ConflictSet& conflictSet);

std::ostream& operator<<(std::ostream& os, const ConflictSet& conflictSet);

#endif // SOPNET_SLICES_CONFLICT_SET_H__

