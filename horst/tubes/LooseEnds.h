#ifndef HOST_FEATURES_LOOSE_ENDS_H__
#define HOST_FEATURES_LOOSE_ENDS_H__

#include <map>
#include <vigra/multi_array.hxx>
#include "LooseEnd.h"

/**
 * Finds and stores loose ends for a set of tubes.
 */
class LooseEnds {

public:

	void findLooseEnds(const vigra::MultiArrayView<3, int>& volume);

	std::vector<LooseEnd>&       getLooseEnds(int label)       { return _looseEnds[label]; }
	const std::vector<LooseEnd>& getLooseEnds(int label) const { return _looseEnds.at(label); }

private:

	std::map<int, std::vector<LooseEnd>> _looseEnds;
};

#endif // HOST_FEATURES_LOOSE_ENDS_H__

