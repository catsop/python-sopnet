#include "Slices.h"

Slices::Slices() :
	_adaptor(0),
	_kdTree(0),
	_kdTreeDirty(true) {}

Slices::Slices(const Slices& other) :
	pipeline::Data(),
	_slices(other._slices),
	_conflicts(other._conflicts),
	_adaptor(0),
	_kdTree(0),
	_kdTreeDirty(true) {}

Slices&
Slices::operator=(const Slices& other) {

	if (_kdTree)
		delete _kdTree;

	if (_adaptor)
		delete _adaptor;

	_kdTree = 0;
	_adaptor = 0;
	_kdTreeDirty = true;

	_slices = other._slices;
	_conflicts = other._conflicts;

	return *this;
}

Slices::~Slices() {

	if (_kdTree)
		delete _kdTree;

	if (_adaptor)
		delete _adaptor;
}

void
Slices::clear() {

	_slices.clear();
	_conflicts.clear();
}

void
Slices::add(boost::shared_ptr<Slice> slice) {

	_slices.insert(slice);

	_kdTreeDirty = true;
}

void
Slices::addAll(const Slices& slices) {

	_slices.insert(slices.begin(), slices.end());

	_kdTreeDirty = true;
}

void
Slices::remove(boost::shared_ptr<Slice> slice) {

	slices_type::iterator i = _slices.find(slice);

	if (i != _slices.end())
		_slices.erase(i);
}

std::vector<boost::shared_ptr<Slice> >
Slices::find(const util::point<double, 2>& center, double distance) {

	if (_kdTreeDirty) {

		delete _adaptor;
		delete _kdTree;

		_adaptor = 0;
		_kdTree = 0;
	}

	// create kd-tree, if it does not exist
	if (!_kdTree) {

		// create slice vector adaptor
		_adaptor = new SliceVectorAdaptor(_slices.begin(), _slices.end());

		// create the tree
		_kdTree = new SliceKdTree(2, *_adaptor, nanoflann::KDTreeSingleIndexAdaptorParams(10));

		// create index
		_kdTree->buildIndex();
	}

	// find close indices
	std::vector<std::pair<size_t, double> > results;

	double query[2];
	query[0] = center.x();
	query[1] = center.y();

	nanoflann::SearchParams params(0 /* ignored parameter */);

	_kdTree->radiusSearch(&query[0], distance, results, params);

	// fill result vector
	std::vector<boost::shared_ptr<Slice> > found;

	for (const auto& pair : results)
		found.push_back((*_adaptor)[pair.first]);

	return found;
}

void Slices::addConflictsFromSlices(const Slices& slices)
{
	
	for (conflicts_type::value_type conflict : slices._conflicts)
	{
		const unsigned int id = conflict.first;
		
		_conflicts[id].reserve(_conflicts[id].size() + conflict.second.size());
		
		_conflicts[id].insert(_conflicts[id].end(), conflict.second.begin(), conflict.second.end());
	}
	
}

std::vector<unsigned int>
Slices::getConflicts(unsigned int id)
{
	if (_conflicts.count(id))
	{
		return _conflicts[id];
	}
	else
	{
		return std::vector<unsigned int>();
	}
}

void
Slices::setConflicts(unsigned int id, std::vector<unsigned int> conflicts)
{
	if (!conflicts.empty())
	{
		_conflicts[id] = conflicts;
	}
}

void
Slices::translate(const util::point<int, 2>& offset) {

	for (boost::shared_ptr<Slice> slice : _slices)
		slice->translate(offset);

	_kdTreeDirty = true;
}
