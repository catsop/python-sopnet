#ifndef COST_WRITER_H__
#define COST_WRITER_H__

#include <pipeline/all.h>
#include <sopnet/segments/Segments.h>
#include <inference/LinearObjective.h>
#include <catmaid/persistence/SegmentStore.h>

class CostWriter : public pipeline::SimpleProcessNode<>
{
public:
	/**
	 * Construct a CostWriter, which pushes LinearObjective objects into a SegmentStore.
	 * Inputs:
	 *   SegmentStore    - "store"
	 *   Segments        - "segments"
	 *   LinearObjective - "objective"
	 * No Outputs.
	 */
	CostWriter();
	
	void writeCosts();
	
private:
	void updateOutputs(){}
	
	pipeline::Input<SegmentStore> _store;
	pipeline::Input<Segments> _segments;
	pipeline::Input<LinearObjective> _objective;

};

#endif //COST_WRITER_H__