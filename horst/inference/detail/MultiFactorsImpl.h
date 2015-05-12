#ifndef HOST_INFERENCE_MULTI_FACTORS_H__
#define HOST_INFERENCE_MULTI_FACTORS_H__

#include <map>
#include <vector>
#include <graph/Graph.h>

namespace host {
namespace detail {

template <typename EdgeType>
class MultiFactorsImpl {

public:

	typedef std::vector<EdgeType>   Edges;
	typedef std::map<Edges,double>  Factors;

	typedef typename Factors::iterator       iterator;
	typedef typename Factors::const_iterator const_iterator;

	inline iterator begin() { return _factors.begin(); }
	inline iterator end()   { return _factors.end(); }
	inline const_iterator begin() const { return _factors.begin(); }
	inline const_iterator end()   const { return _factors.end(); }

	inline double& operator[](const Edges& edges) { return _factors[edges]; }

	inline size_t size() const { return _factors.size(); }

private:

	Factors _factors;
};

} // namespace host
} // namespace detail

#endif // HOST_INFERENCE_MULTI_FACTORS_H__

