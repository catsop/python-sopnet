#ifndef SOLUTION_WRITER_H__
#define SOLUTION_WRITER_H__

#include <pipeline/all.h>
#include <catmaid/persistence/SegmentStore.h>
#include <sopnet/segments/Segments.h>
#include <inference/Solution.h>
#include <sopnet/block/Cores.h>

class SolutionWriter : public pipeline::SimpleProcessNode<>
{
public:
	/**
	 * Construct a SolutionWriter
	 * Inputs:
	 *   Segments     - "segments"
	 *   Cores         - "cores"
	 *   Solution     - "solution"
	 *   SegmentStore - "store"
	 * No Outputs.
	 */
	SolutionWriter();

	/**
	 * Write the solution scores for a given set of Segments in a Core to the SegmentStore. The
	 * Segments must already exist in the store for their scores to be recorded.
	 */
	void writeSolution();
	
private:
	void updateOutputs() {}
	
	bool segmentOverlapsCore(boost::shared_ptr<Segment> segment,
							 const util::rect<int>& coreRect);

	pipeline::Input<Segments> _segments;
	pipeline::Input<Solution> _solution;
	pipeline::Input<Cores> _cores;
	pipeline::Input<SegmentStore> _store;
};

#endif //SOLUTION_WRITER_H__