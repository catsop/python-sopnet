#ifndef HOST_GRAPHS_WEIGHTED_GRAPH_WRITER_H__
#define HOST_GRAPHS_WEIGHTED_GRAPH_WRITER_H__

#include <lemon/lgf_writer.h>

namespace host {

class WeightedGraphWriter {

public:

	WeightedGraphWriter(const std::string& filename) :
		_filename(filename) {}

	void write(const host::Graph& graph, const host::ArcWeights& weights) {

		host::ArcSelection dummy(graph);
		write(graph, weights, dummy);
	}

	void write(const host::Graph& graph, const host::ArcWeights& weights, const host::ArcSelection& arcSelection) {

		std::ofstream os(_filename.c_str());

		if (_filename.find(".lgf") != std::string::npos) {

			lemon::digraphWriter(graph, os).arcMap("weight", weights).arcMap("mst", arcSelection).run();

		} else { // write in GUESS format

			os << "nodedef>name VARCHAR" << std::endl;

			for (host::Graph::NodeIt node(graph); node != lemon::INVALID; ++node)
				os << graph.id(node) << std::endl;

			os << "arcdef>node1 VARCHAR,node2 VARCHAR,weight DOUBLE,mst BOOLEAN" << std::endl;

			for (host::Graph::ArcIt arc(graph); arc != lemon::INVALID; ++arc)
				os
						<< graph.id(graph.source(arc)) << ","
						<< graph.id(graph.target(arc)) << ","
						<< weights[arc] << ","
						<< arcSelection[arc] << std::endl;
		}
	}

private:

	std::string _filename;
};

} // namespace host

#endif // HOST_GRAPHS_WEIGHTED_GRAPH_WRITER_H__

