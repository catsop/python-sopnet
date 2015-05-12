#ifndef HOST_GRAPHS_WEIGHTED_GRAPH_READER_H__
#define HOST_GRAPHS_WEIGHTED_GRAPH_READER_H__

#include "WeightedGraphGenerator.h"

namespace host {

std::istream& operator>>(std::istream& is, host::ArcType& type);

class WeightedGraphReader : public WeightedGraphGenerator {

public:

	WeightedGraphReader(const std::string& filename) :
		_filename(filename) {}

	void fill(
			host::Graph& graph,
			host::ArcWeights& weights,
			host::ArcLabels& labels,
			host::ArcTypes& types);

private:

	void addOppositeArc(
			host::Graph& graph,
			const host::Arc& arc,
			host::ArcWeights& weights,
			host::ArcLabels& labels,
			host::ArcTypes& types);

	std::string _filename;
};

} // namespace host

#endif // HOST_GRAPHS_WEIGHTED_GRAPH_READER_H__

