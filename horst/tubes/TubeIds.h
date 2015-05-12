#ifndef HOST_TUBES_TUBE_IDS_H__
#define HOST_TUBES_TUBE_IDS_H__

#include <vector>

/**
 * Represents a collection of tubes by their ids.
 */
class TubeIds {

public:

	typedef typename std::vector<TubeId>::iterator       iterator;
	typedef typename std::vector<TubeId>::const_iterator const_iterator;

	void add(TubeId id) { _tubes.push_back(id); }

	bool remove(TubeId id) {

		iterator i = std::find(begin(), end(), id);

		if (i == end())
			return false;

		_tubes.erase(i);
		return true;
	}

	std::size_t size() const { return _tubes.size(); }

	/**
	 * Direct iterator access to the stored tube ids.
	 */
	iterator begin() { return _tubes.begin(); }
	const_iterator begin() const { return _tubes.begin(); }
	iterator end() { return _tubes.end(); }
	const_iterator end() const { return _tubes.end(); }

private:

	std::vector<TubeId> _tubes;
};

#endif // HOST_TUBES_TUBE_IDS_H__

