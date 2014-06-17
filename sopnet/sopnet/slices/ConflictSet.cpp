#include "ConflictSet.h"

std::size_t hash_value(const ConflictSet& conflictSet)
{
	std::size_t hv = 0;
	
	foreach (unsigned int id, conflictSet.getSlices())
	{
		boost::hash_combine(hv, boost::hash_value(id));
	}
	
	return hv;
}

std::ostream& operator<<(std::ostream& os, const ConflictSet& conflictSet)
{
	os << "ConflictSet size " << conflictSet.getSlices().size() << ": ";
	foreach (unsigned id, conflictSet.getSlices())
	{
		os << id << " ";
	}
	return os;
}
