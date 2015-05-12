#include "Logging.h"

namespace host {

	const Graph* LoggingState::_graph = 0;

} // namespace host

std::ostream&
operator<<(std::ostream& os, const std::vector<host::Arc>& arcs) {

	bool first = true;

	os << "[";
	for (const auto& arc : arcs) {

		if (!first)
			os << ",";

		os << arc;
		first = false;
	}
	os << "]";

	return os;
}

std::ostream&
operator<<(std::ostream& os, const std::vector<host::Edge>& edges) {

	bool first = true;

	os << "[";
	for (const auto& edge : edges) {

		if (!first)
			os << ",";

		os << edge;
		first = false;
	}
	os << "]";

	return os;
}

std::ostream&
operator<<(std::ostream& os, const std::set<host::Edge>& edges) {

	bool first = true;

	os << "{";
	for (const auto& edge : edges) {

		if (!first)
			os << ",";

		os << edge;
		first = false;
	}
	os << "}";

	return os;
}
