#ifndef CELLTRACKER_RECONSTRUCTOR_H__
#define CELLTRACKER_RECONSTRUCTOR_H__

#include <pipeline/all.h>
#include <inference/Solution.h>
#include "Segments.h"

class Reconstructor : public pipeline::SimpleProcessNode {

public:

	Reconstructor();

private:

	void updateOutputs();

	void updateReconstruction();

	template <typename SegmentType>
	void probe(boost::shared_ptr<SegmentType> segment);

	pipeline::Input<Solution>                               _solution;
	pipeline::Input<std::map<unsigned int, unsigned int> >  _segmentIdsMap;
	pipeline::Input<Segments>                               _segments;
	pipeline::Output<Segments>                              _reconstruction;
};

#endif // CELLTRACKER_RECONSTRUCTOR_H__
