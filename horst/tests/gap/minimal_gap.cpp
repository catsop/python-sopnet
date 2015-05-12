#include <tests.h>
#include <io/WeightedGraphReader.h>
#include <inference/ExplicitWeightTerm.h>
#include <inference/CandidateConflictTerm.h>
#include <inference/HostSearch.h>

void minimal_gap() {

	boost::filesystem::path dataDir = dir_of(__FILE__);
	boost::filesystem::path graphfile = dataDir/"minimal_gap.dat";

	host::Graph            graph;
	host::ArcWeights       weights(graph);
	host::ArcLabels        labels(graph);
	host::ArcTypes         types(graph);

	host::WeightedGraphReader reader(graphfile.native());
	reader.fill(graph, weights, labels, types);

	unsigned int numArcs = 0;
	for (host::ArcIt arc(graph); arc != lemon::INVALID; ++arc)
		numArcs++;
	BOOST_CHECK_EQUAL(numArcs, 16);

	host::ExplicitWeightTerm    edgeWeightsTerm(graph, weights);
	host::CandidateConflictTerm conflictsTerm(graph, types);

	host::HostSearch search(graph);
	search.addTerm(&edgeWeightsTerm);
	search.addTerm(&conflictsTerm);

	host::ArcSelection mst(graph);
	double value;
	bool optimal;

	LOG_DEBUG(testslog) << std::endl;
	LOG_DEBUG(testslog) << std::endl;
	LOG_DEBUG(testslog) << "running minimal_gap without chain terms" << std::endl;
	LOG_DEBUG(testslog) << std::endl;
	LOG_DEBUG(testslog) << std::endl;

	optimal = search.find(mst, value);

	// check if the correct solution was found
	BOOST_CHECK(!optimal);

	// this would be the optimal value:
	//BOOST_CHECK_CLOSE(value, 0.2, 1e-6);
}

