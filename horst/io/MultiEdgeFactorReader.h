#ifndef HOST_IO_MULTI_EDGE_FACTOR_READER_H__
#define HOST_IO_MULTI_EDGE_FACTOR_READER_H__

#include <graph/Graph.h>
#include <inference/MultiEdgeFactors.h>

namespace host {

class MultiEdgeFactorReader {

public:

	MultiEdgeFactorReader(std::string filename) :
		_filename(filename) {}

	void fill(
			const Graph&      graph,
			const ArcLabels&  labels,
			MultiEdgeFactors& factors);

private:

	std::string _filename;
};

} // namesapce host

#endif // HOST_IO_MULTI_EDGE_FACTOR_READER_H__

