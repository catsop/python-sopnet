#ifndef HOST_GRAPHS_LOGGING_H__
#define HOST_GRAPHS_LOGGING_H__

#include <vector>
#include <set>
#include <iostream>
#include <util/exceptions.h>
#include "Graph.h"

std::ostream& operator<<(std::ostream& os, const std::vector<host::Arc>& arcs);
std::ostream& operator<<(std::ostream& os, const std::vector<host::Edge>& edges);
std::ostream& operator<<(std::ostream& os, const std::set<host::Edge>& edges);

namespace host {

class LoggingState {

public:

	static void setLoggingGraph(const host::Graph* graph) { _graph = graph; }

	static const host::Graph* getLoggingGraph() {
	
		if (_graph == 0)
			UTIL_THROW_EXCEPTION(
					UsageError,
					"no graph was set via operator<< for stream output");

		return _graph;
	}
private:

	static const host::Graph* _graph;
};

template <typename CharT, typename Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const Graph& graph) {

	LoggingState::setLoggingGraph(&graph);
	return os;
}

template <typename CharT, typename Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const Edge& edge) {

	const Graph& graph = *LoggingState::getLoggingGraph();

	// edges are sets of nodes
	os << "{" << graph.id(graph.source(edge)) << ", " << graph.id(graph.target(edge)) << "}";
	return os;
}

} // namespace host

namespace lemon {

template <typename CharT, typename Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const host::Arc& arc) {

	const host::Graph& graph = *host::LoggingState::getLoggingGraph();

	// arcs are tuples of nodes
	os << "(" << graph.id(graph.source(arc)) << ", " << graph.id(graph.target(arc)) << ")";
	return os;
}

template <typename CharT, typename Traits>
std::basic_ostream<CharT, Traits>& operator<<(std::basic_ostream<CharT, Traits>& os, const host::Edge& edge) {

	const host::Graph& graph = *host::LoggingState::getLoggingGraph();

	// edges are sets of nodes
	os << "{" << graph.id(graph.source(edge)) << ", " << graph.id(graph.target(edge)) << "}";
	return os;
}

} // namespace lemon

#endif // HOST_GRAPHS_LOGGING_H__

