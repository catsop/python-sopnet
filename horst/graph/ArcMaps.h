#ifndef HOST_GRAPH_ARC_MAPS_H__
#define HOST_GRAPH_ARC_MAPS_H__

#include "Graph.h"

namespace host {

typedef GraphBase::ArcMap<std::string> ArcLabels;
typedef GraphBase::ArcMap<ArcType>     ArcTypes;

class ArcWeights : public GraphBase::ArcMap<double> {

public:

	class WeightsProxy {

	public:

		WeightsProxy(ArcWeights* weights, const Edge* edge) :
			_weights(weights),
			_edge(edge) {}

		template <typename T>
		WeightsProxy& operator+=(const T& value) {

			for (const auto& arc : *_edge)
				(*_weights)[arc] += value;

			return *this;
		}

		template <typename T>
		WeightsProxy& operator-=(const T& value) {

			for (const auto& arc : *_edge)
				(*_weights)[arc] -= value;

			return *this;
		}

	private:

		ArcWeights* _weights;
		const Edge* _edge;
	};

	ArcWeights(const Graph& graph) :
		Graph::ArcMap<double>(graph) {}

	using Graph::ArcMap<double>::operator[];

	WeightsProxy operator[](const Edge& edge) {

		return WeightsProxy(this, &edge);
	}
};

/**
 * Selection of arcs, represented as bool attributes.
 */
class ArcSelection : public GraphBase::ArcMap<bool> {

public:

	ArcSelection(const Graph& graph) :
		Graph::ArcMap<bool>(graph) {}

	using Graph::ArcMap<bool>::operator[];

	/**
	 * Check whether any of the arcs of the given edge is contained in the 
	 * selection.
	 */
	bool operator[](const Edge& edge) const {

		for (const Arc& arc : edge)
			if ((*this)[arc])
				return true;

		return false;
	}
};

} // namespace host

#endif // HOST_GRAPH_ARC_MAPS_H__

