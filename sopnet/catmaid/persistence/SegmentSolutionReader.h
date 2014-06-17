#ifndef SOLUTION_READER_H__
#define SOLUTION_READER_H__

#include <pipeline/all.h>
#include <catmaid/persistence/SegmentStore.h>
#include <sopnet/segments/Segments.h>
#include <inference/Solution.h>

class SegmentSolutionReader : public pipeline::SimpleProcessNode<>
{
public:
	/**
	 * ProcessNode that interfaces with a SegmentStore to read a Solution for
	 * a given set of Segments
	 * 
	 * Inputs:
	 *   SegmentStore - "store"
	 *   Core         - "core"
	 *   Segments     - "segments"
	 * Outputs:
	 *   Solution     - "solution"
	 */
	SegmentSolutionReader();
	
private:
	void updateOutputs();
	
	pipeline::Input<SegmentStore> _store;
	pipeline::Input<Core> _core;
	pipeline::Input<Segments> _segments;
	pipeline::Output<Solution> _solution;
	
};

#endif //SOLUTION_READER_H__