#include <util/assert.h>
#include "Hdf5GraphReader.h"

void
Hdf5GraphReader::readGraph(Graph& graph) {

	if (!_hdfFile.existsDataset("num_nodes"))
		return;

	vigra::ArrayVector<int> nodes;
	vigra::ArrayVector<int> edges; // stored in pairs

	_hdfFile.readAndResize(
			"num_nodes",
			nodes);
	int numNodes = nodes[0];

	if (numNodes == 0)
		return;

	if (_hdfFile.existsDataset("edges"))
		_hdfFile.readAndResize(
				"edges",
				edges);

	for (int i = 0; i < numNodes; i++) {

		Graph::Node node = graph.addNode();
		UTIL_ASSERT_REL(graph.id(node), ==, i);
	}

	for (unsigned int i = 0; i < edges.size(); i += 2) {

		Graph::Node u = graph.nodeFromId(edges[i]);
		Graph::Node v = graph.nodeFromId(edges[i+1]);

		graph.addEdge(u, v);
	}
}
