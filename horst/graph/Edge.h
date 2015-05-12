#ifndef HOST_GRAPH_EDGE_H__
#define HOST_GRAPH_EDGE_H__

#include "GraphBase.h"

namespace host {

/**
 * An edge consists of all arcs between two nodes (i.e., one or two).
 */
class Edge {

public:

	typedef std::vector<Arc>     Arcs;
	typedef Arcs::iterator       iterator;
	typedef Arcs::const_iterator const_iterator;

	void addArc(const Arc& arc) { _arcs.push_back(arc); std::sort(_arcs.begin(), _arcs.end()); }

	iterator begin() { return _arcs.begin(); }
	const_iterator begin() const { return _arcs.begin(); }
	iterator end() { return _arcs.end(); }
	const_iterator end() const { return _arcs.end(); }

	size_t size() const { return _arcs.size(); }

	const Arc& operator[](size_t i) const { return _arcs[i]; }
	Arc& operator[](size_t i) { return _arcs[i]; }

	bool operator==(const Edge& other) const { return _arcs == other._arcs; }

	bool operator<(const Edge& other) const { return *_arcs.begin() < *other._arcs.begin(); }

	bool contains(const Arc& arc) { return std::find(_arcs.begin(), _arcs.end(), arc) != _arcs.end(); }

private:

	Arcs _arcs;
};

} // namespace host

#endif // HOST_GRAPH_EDGE_H__

