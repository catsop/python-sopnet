#ifndef CORE_SOLVER_H__
#define CORE_SOLVER_H__

#include <boost/shared_ptr.hpp>

#include <pipeline/all.h>

#include <catmaid/persistence/SegmentStore.h>
#include <catmaid/persistence/SliceStore.h>
#include <catmaid/persistence/StackStore.h>
#include <inference/PriorCostFunctionParameters.h>
#include <inference/ProblemAssembler.h>
#include <inference/SegmentationCostFunctionParameters.h>
#include <sopnet/block/Blocks.h>
#include <sopnet/segments/SegmentTrees.h>

class CoreSolver : public pipeline::SimpleProcessNode<>
{
public:
	CoreSolver();
	
private:
	class ConstraintAssembler : public pipeline::SimpleProcessNode<>
	{
	public:
		ConstraintAssembler();
		
	private:
		void updateOutputs();
		
		void assembleConstraint(const ConflictSet& conflictSet,
						std::map<unsigned int, std::vector<unsigned int> >& sliceSegmentMap);
		
		pipeline::Input<Segments> _segments;
		pipeline::Input<ConflictSets> _conflictSets;
		pipeline::Input<bool> _assemblerForceExplanation;
		
		pipeline::Output<LinearConstraints> _constraints;
	};
	
	class EndExtractor : public pipeline::SimpleProcessNode<>
	{
	public:
		EndExtractor();
		
	private:
		void updateOutputs();
		
		pipeline::Input<Segments> _eeSegments;
		pipeline::Input<Slices> _eeSlices;
		
		pipeline::Output<Segments> _allSegments;
	};
	
	void updateOutputs();
	pipeline::Value<Blocks> computeBound(
		const boost::shared_ptr<ProblemAssembler> problemAssembler);
	
	pipeline::Input<PriorCostFunctionParameters> _priorCostFunctionParameters;
	pipeline::Input<SegmentationCostFunctionParameters> _segmentationCostFunctionParameters;
	pipeline::Input<Blocks> _blocks;
	pipeline::Input<SegmentStore> _segmentStore;
	pipeline::Input<SliceStore> _sliceStore;
	pipeline::Input<StackStore> _rawImageStore;
	pipeline::Input<StackStore> _membraneStore;
	pipeline::Input<bool> _forceExplanation;
	
	pipeline::Output<SegmentTrees> _neurons;
	pipeline::Output<Segments> _outputSegments;
	

};

#endif //CORE_SOLVER_H__
