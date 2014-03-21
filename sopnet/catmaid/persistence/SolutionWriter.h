#ifndef SOLUTION_WRITER_H__
#define SOLUTION_WRITER_H__

#include <pipeline/all.h>
#include <catmaid/persistence/SegmentStore.h>
#include <sopnet/segments/Segments.h>
#include <inference/Solution.h>

class SolutionWriter : public pipeline::SimpleProcessNode<>
{
public:
	/**
	 * Construct a SolutionWriter
	 * Inputs:
	 *   Segments     - "segments"
	 *   Solution     - "solution"
	 *   SegmentStore - "store"
	 * No Outputs.
	 */
	SolutionWriter();

	/**
	 * Write the solution scores for a given set of Segments to the SegmentStore. The Segments
	 * must already exist in the store for their scores to be recorded.
	 */
	void writeSolution();
	
private:
	void updateOutputs() {}

	pipeline::Input<Segments> _segments;
	pipeline::Input<Solution> _solution;
	pipeline::Input<SegmentStore> _store;
};

#endif //SOLUTION_WRITER_H__