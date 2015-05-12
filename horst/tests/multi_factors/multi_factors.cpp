#include <tests.h>
#include <io/WeightedGraphReader.h>
#include <io/MultiEdgeFactorReader.h>
#include <inference/HostSearch.h>
#include <inference/ExplicitWeightTerm.h>
#include <inference/CandidateConflictTerm.h>
#include <inference/MultiEdgeFactorTerm.h>

void multi_factors() {

	boost::filesystem::path dataDir = dir_of(__FILE__);
	boost::filesystem::path graphfile = dataDir/"small_graph.dat";
	boost::filesystem::path factorsfile = dataDir/"small_graph_factors.dat";

	host::Graph            graph;
	host::ArcWeights       weights(graph);
	host::ArcLabels        labels(graph);
	host::ArcTypes         types(graph);

	host::WeightedGraphReader reader(graphfile.native());
	reader.fill(graph, weights, labels, types);

	host::MultiEdgeFactors multiEdgeFactors;
	host::MultiEdgeFactorReader factorReader(factorsfile.native());
	factorReader.fill(graph, labels, multiEdgeFactors);

	host::ExplicitWeightTerm    edgeWeightsTerm(graph, weights);
	host::MultiEdgeFactorTerm   multiFactorsTerm(graph, multiEdgeFactors);

	host::HostSearch search(graph);
	search.addTerm(&edgeWeightsTerm);
	search.addTerm(&multiFactorsTerm);

	host::ArcSelection mst(graph);
	double value;
	bool optimal = search.find(mst, value);

	// check if the correct solution was found
	BOOST_CHECK(optimal);
	BOOST_CHECK_CLOSE(value, -3, 1e-6);
}
