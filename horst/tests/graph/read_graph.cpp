#include <map>
#include <tests.h>
#include <io/WeightedGraphReader.h>

namespace read_graph_case {

unsigned int degree(const host::Graph& graph, const host::Node& node) {

	unsigned int degree = 0;
	for (host::OutArcIt out(graph, node); out != lemon::INVALID; ++out)
		degree++;

	return degree;
}

} using namespace read_graph_case;

void read_graph() {

	boost::filesystem::path dataDir = dir_of(__FILE__);

	{
		boost::filesystem::path graphfile = dataDir/"star.dat";

		host::Graph            graph;
		host::ArcWeights       arcWeights(graph);
		host::ArcLabels        arcLabels(graph);
		host::ArcTypes         arcTypes(graph);

		host::WeightedGraphReader reader(graphfile.native());
		reader.fill(graph, arcWeights, arcLabels, arcTypes);

		BOOST_CHECK_EQUAL(graph.isUndirected(), false);
		BOOST_CHECK_EQUAL(graph.id(graph.getRoot()), 0);
		BOOST_CHECK_EQUAL(degree(graph, graph.getRoot()), 5);

		unsigned int numArcs = 0;
		for (host::ArcIt arc(graph); arc != lemon::INVALID; ++arc) {

			numArcs++;

			BOOST_CHECK_EQUAL(arcWeights[arc], 0.5);
			BOOST_CHECK_EQUAL(arcLabels[arc][0],  static_cast<char>('a' + graph.id(arc)));
			BOOST_CHECK_EQUAL(arcTypes[arc], host::Link);

			host::Edge edge = graph.edgeFromArc(arc);
			BOOST_CHECK_EQUAL(edge.size(), 1);
			BOOST_CHECK(edge.contains(arc));

			host::Edge copy = edge;
			BOOST_CHECK(edge == copy);
		}

		BOOST_CHECK_EQUAL(numArcs, 5);
	}

	{
		boost::filesystem::path graphfile = dataDir/"undirected.dat";

		host::Graph            graph;
		host::ArcWeights       arcWeights(graph);
		host::ArcLabels        arcLabels(graph);
		host::ArcTypes         arcTypes(graph);

		host::WeightedGraphReader reader(graphfile.native());
		reader.fill(graph, arcWeights, arcLabels, arcTypes);

		BOOST_CHECK_EQUAL(graph.isUndirected(), true);
		BOOST_CHECK_EQUAL(graph.id(graph.getRoot()), 0);
		BOOST_CHECK_EQUAL(degree(graph, graph.getRoot()), 5);

		std::map<host::Arc, host::Edge> arcToEdge;

		unsigned int numArcs = 0;
		for (host::ArcIt arc(graph); arc != lemon::INVALID; ++arc) {

			numArcs++;

			BOOST_CHECK_EQUAL(arcWeights[arc], 0.5);

			if (graph.id(arc) < 5) {

				std::string label = {static_cast<char>('a' + (graph.id(arc)%5))};
				BOOST_CHECK_EQUAL(arcLabels[arc], label);
			}

			BOOST_CHECK_EQUAL(arcTypes[arc], host::Link);

			host::Edge edge = graph.edgeFromArc(arc);
			BOOST_CHECK_EQUAL(edge.size(), 2);
			BOOST_CHECK(edge.contains(arc));

			BOOST_CHECK_EQUAL(arcLabels[edge[0]] + "_opp", arcLabels[edge[1]]);
			BOOST_CHECK_EQUAL(arcWeights[edge[0]], arcWeights[edge[1]]);
			BOOST_CHECK_EQUAL(arcTypes[edge[0]], arcTypes[edge[1]]);

			if (arcToEdge.count(arc)) {

				host::Edge& copy = arcToEdge[arc];
				BOOST_CHECK(copy == edge);
			}

			arcToEdge[arc] = edge;
		}

		BOOST_CHECK_EQUAL(numArcs, 10);
	}

	{
		boost::filesystem::path graphfile = dataDir/"conflicts.dat";

		host::Graph            graph;
		host::ArcWeights       arcWeights(graph);
		host::ArcLabels        arcLabels(graph);
		host::ArcTypes         arcTypes(graph);

		host::WeightedGraphReader reader(graphfile.native());
		reader.fill(graph, arcWeights, arcLabels, arcTypes);

		BOOST_CHECK_EQUAL(graph.isUndirected(), true);
		BOOST_CHECK_EQUAL(graph.id(graph.getRoot()), 0);
		BOOST_CHECK_EQUAL(degree(graph, graph.getRoot()), 7);

		std::map<host::Arc, host::Edge> arcToEdge;

		unsigned int numArcs = 0;
		for (host::ArcIt arc(graph); arc != lemon::INVALID; ++arc) {

			numArcs++;

			if (arcTypes[arc] == host::Link)
				BOOST_CHECK_EQUAL(arcWeights[arc], 0.5);
			else
				BOOST_CHECK_EQUAL(arcWeights[arc], 0);

			host::Edge edge = graph.edgeFromArc(arc);
			BOOST_CHECK_EQUAL(edge.size(), 2);
			BOOST_CHECK(edge.contains(arc));

			BOOST_CHECK_EQUAL(arcLabels[edge[0]] + "_opp", arcLabels[edge[1]]);
			BOOST_CHECK_EQUAL(arcWeights[edge[0]], arcWeights[edge[1]]);
			BOOST_CHECK_EQUAL(arcTypes[edge[0]], arcTypes[edge[1]]);

			if (arcToEdge.count(arc)) {

				host::Edge& copy = arcToEdge[arc];
				BOOST_CHECK(copy == edge);
			}

			arcToEdge[arc] = edge;
		}

		BOOST_CHECK_EQUAL(numArcs, 56);
	}
}
