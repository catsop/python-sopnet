#ifndef COST_READER_H__
#define COST_READER_H__

#include <pipeline/all.h>
#include <sopnet/segments/Segments.h>
#include <inference/LinearObjective.h>
#include <catmaid/persistence/SegmentStore.h>

class CostReader : public pipeline::SimpleProcessNode<>
{
public:
	/**
	 * Construct a CostReader, which retrieves LinearObjectives from a SegmentStore
	 * Inputs:
	 *   SegmentStore - "store"
	 *   Segments     - "segments"
	 *   double       - "default cost" - Optional
	 * Outputs:
	 *   LinearObjective - "objective"
	 */
	CostReader();
	
private:
	void updateOutputs();
	
	pipeline::Input<SegmentStore> _store;
	pipeline::Input<Segments> _segments;
	pipeline::Input<double> _defaultCost;
	pipeline::Output<LinearObjective> _objective;
};

#endif //COST_READER_H__