#ifndef HOST_GRAPH_GRAPH_H__
#define HOST_GRAPH_GRAPH_H__

#include <lemon/list_graph.h>
#include "Node.h"
#include "Arc.h"
#include "Edge.h"

namespace host {

class Graph : public GraphBase {

public:

	Graph() :
		_isUndirected(false) {}

	/**
	 * Returns true if this graph is undirected. Undirected graphs have 
	 * symmetric link arcs, i.e., whenever node A links to node B with cost c, 
	 * node B does also link to node A with the same costs c.
	 */
	bool isUndirected() { return _isUndirected; }

	/**
	 * Mark this graph as undirected. This does not change the graph and does 
	 * not test whether the arcs are really symmetric. Set this if you created 
	 * the graph and know whether it is directed or not.
	 */
	void setUndirected(bool isUndirected) { _isUndirected = isUndirected; }

	/**
	 * Set the root node for the branching search in directed graphs.
	 */
	void setRoot(Node root) { _root = root; }

	/**
	 * Get the root node.
	 */
	Node getRoot() const { return _root; }

	/**
	 * The "source" of an undirected edge, i.e., the lower of the two nodes.
	 */
	Node source(const Edge& edge) const { return source(*edge.begin()); }
	using GraphBase::source;

	/**
	 * The "target" of an undirected edge, i.e., the bigger of the two nodes.
	 */
	Node target(const Edge& edge) const { return target(*edge.begin()); }
	using GraphBase::target;

	/**
	 * Find the (undirected) edge for a given (directed) arc.
	 */
	Edge edgeFromArc(Arc arc) const {

		Edge edge;
		edge.addArc(arc);

		// add reverse arc, if it exists
		if (reverseArc(arc))
			edge.addArc(arc);

		return edge;
	}

	/**
	 * Replace arc by its reverse arc, if it exists. If it does not exist, arc 
	 * remains unchanged and false is returned.
	 */
	bool reverseArc(Arc& arc) const {

		for (OutArcIt out(*this, target(arc)); out != lemon::INVALID; ++out)
			if (target(out) == source(arc)) {

				arc = out;
				return true;
			}
		return false;
	}

private:

	bool _isUndirected;

	Node _root;
};

} // namespace host

#include "ArcMaps.h"

#endif // HOST_GRAPH_GRAPH_H__

