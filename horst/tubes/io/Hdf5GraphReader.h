#ifndef HOST_TUBES_IO_HDF5_GRAPH_READER_H__
#define HOST_TUBES_IO_HDF5_GRAPH_READER_H__

#include <vigra/hdf5impex.hxx>
#include <lemon/list_graph.h>

class Hdf5GraphReader {

public:

	/**
	 * Converts numeric types into array-like objects for HDF5 storage.
	 */
	template <typename T>
	struct DefaultConverter {

		typedef T ArrayValueType;
		static const int ArraySize = 1;

		T operator()(const vigra::ArrayVectorView<T>& array) const {

			return array[0];
		}
	};

	typedef lemon::ListGraph Graph;

	template <typename ValueType>
	using NodeMap = Graph::NodeMap<ValueType>;
	template <typename ValueType>
	using EdgeMap = Graph::EdgeMap<ValueType>;

	Hdf5GraphReader(vigra::HDF5File& hdfFile) :
		_hdfFile(hdfFile) {}

	/**
	 * Read the graph structure in the current group with datasets "nodes" and 
	 * "edges".
	 */
	void readGraph(Graph& graph);

	/**
	 * Read a node map from a dataset with the given name. A converter object 
	 * needs to be provided to transform ArrayVectorView<T> objects into 
	 * ValueType. Converter has to define:
	 *
	 *   Converter::ArrayValueType
	 *
	 *     the expected type T of the ArrayVectorView<T>
	 *
	 *   Converter::ArraySize
	 *
	 *     the number of elements in the array
	 *
	 *   ValueType operator()(const ArrayVectorView<T>& a)
	 *
	 *     the conversion operator
	 */
	template <typename ValueType, typename Converter = DefaultConverter<ValueType>>
	void readNodeMap(
			const Graph&        graph,
			NodeMap<ValueType>& map,
			std::string         name,
			const Converter&    converter = Converter()) {

		typedef vigra::ArrayVector<typename Converter::ArrayValueType> ArrayType;

		ArrayType values;

		if (_hdfFile.existsDataset(name))
			_hdfFile.readAndResize(
					name,
					values);

		for (Graph::NodeIt node(graph); node != lemon::INVALID; ++node) {

			std::size_t i = graph.id(node);
			map[node] = converter(values.subarray(i*Converter::ArraySize, (i + 1)*Converter::ArraySize));
		}
	}

	/**
	 * Stores an edge map in a dataset with the given name.
	 */
	template <typename ValueType>
	void readEdgeMap(const EdgeMap<ValueType>& map, std::string name);

	/**
	 * Read an edge map with entries of variable length from a dataset with the 
	 * given name. ContainerType has to define:
	 *
	 *   ContainerType::clear()
	 *
	 *     empty the container
	 *
	 *   ContainerType::push_back(ContainerType::value_type)
	 *
	 *     add an element to the container
	 *
	 *   ContainerType::value_type
	 *
	 *     type of the elements in the collection
	 *
	 *   ContainerType::size()
	 *
	 *     number of elements in the collection
	 *
	 * A converter object needs to be provided to transform ArrayVectorView<T> 
	 * into ContainerType::value_type. Converter has to define:
	 *
	 *   Converter::ArrayValueType
	 *
	 *     the expected type T of the ArrayVectorView<T>
	 *
	 *   Converter::ArraySize
	 *
	 *     the number of elements in the array
	 *
	 *   ValueType operator()(const ArrayVectorView<T>& a)
	 *
	 *     the conversion operator
	 */
	template <typename ContainerType, typename Converter>
	void readVarLengthEdgeMap(
			const Graph&            graph,
			EdgeMap<ContainerType>& map,
			std::string             name,
			const Converter&        converter) {

		typedef typename ContainerType::value_type                     ValueType;
		typedef vigra::ArrayVector<typename Converter::ArrayValueType> ArrayType;

		ArrayType               values;
		vigra::ArrayVector<int> chunks;

		if (_hdfFile.existsDataset(name + "_values"))
			_hdfFile.readAndResize(
					name + "_values",
					values);

		if (_hdfFile.existsDataset(name + "_chunks"))
			_hdfFile.readAndResize(
					name + "_chunks",
					chunks);

		if (values.size() == 0 || chunks.size() == 0)
			return;

		int edgeNum     = 0;
		int nextElement = 0;
		for (Graph::EdgeIt edge(graph); edge != lemon::INVALID; ++edge) {

			map[edge].clear();
			int numElements = chunks[edgeNum];

			for (int i = 0; i < numElements; i++) {

				map[edge].push_back(converter(values.subarray(nextElement, nextElement + Converter::ArraySize)));
				nextElement += Converter::ArraySize;
			}

			edgeNum++;
		}
	}
private:

	vigra::HDF5File& _hdfFile;
};

#endif // HOST_TUBES_IO_HDF5_GRAPH_READER_H__

