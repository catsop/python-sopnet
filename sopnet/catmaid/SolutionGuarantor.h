#ifndef SOLUTION_GUARANTOR_H__
#define SOLUTION_GUARANTOR_H__

#include <boost/shared_ptr.hpp>

#include <pipeline/all.h>

#include <catmaid/persistence/SegmentStore.h>
#include <catmaid/persistence/SliceStore.h>
#include <catmaid/persistence/StackStore.h>
#include <inference/PriorCostFunctionParameters.h>
#include <inference/ProblemAssembler.h>
#include <inference/SegmentationCostFunctionParameters.h>
#include <sopnet/block/Cores.h>
#include <sopnet/segments/SegmentTrees.h>

class SolutionGuarantor : public pipeline::SimpleProcessNode<>
{
public:
	/**
	 * Construct a SolutionGuarantor
	 * Inputs:
	 *   PriorCostFunctionParameters "prior cost parameters"
	 *   Cores                       "cores"
	 *   SegmentationCostFunctionParameters
	 *                               "segmentation cost parameters" - optional
	 *   SegmentStore                "segment store"
	 *   SliceStore                  "slice store"
	 *   StackStore                  "raw stack store"
	 *   StackStore                  "membrane stack store"
	 *   bool                        "force explanation"
	 *   unsigned int                "buffer"
	 * Outputs:
	 *   Blocks                      "need blocks"
	 * 
	 * This process node takes the Slices and Segments from their given stores and Blocks, and
	 * computes a Sopnet segmentation solution over them, given the various other inputs. The
	 * solution is written to a SegmentStore view a SolutionWriter.
	 */
	SolutionGuarantor();
	
	pipeline::Value<Blocks> guaranteeSolution();
	
	static boost::shared_ptr<Blocks> bufferCore(boost::shared_ptr<Core> core,
											  const unsigned int buffer);
	
	static boost::shared_ptr<Blocks> bufferCores(boost::shared_ptr<Cores> cores,
											   const unsigned int buffer);
private:
	/**
	 * Assembles segment-wise linear constraints from slices-wise conflict sets.
	 */
	class ConstraintAssembler : public pipeline::SimpleProcessNode<>
	{
	public:
		ConstraintAssembler();
		
	private:
		void updateOutputs();
		
		boost::shared_ptr<LinearConstraint> assembleConstraint(const ConflictSet& conflictSet,
						std::map<unsigned int, std::vector<unsigned int> >& sliceSegmentMap);
		
		pipeline::Input<Segments> _segments;
		pipeline::Input<ConflictSets> _conflictSets;
		pipeline::Input<bool> _assemblerForceExplanation;
		
		pipeline::Output<LinearConstraints> _constraints;
	};
	
	class LinearObjectiveAssembler : public pipeline::SimpleProcessNode<>
	{
	public:
		LinearObjectiveAssembler();
		
	private:
		void updateOutputs();
		
		pipeline::Input<SegmentStore> _store;
		pipeline::Input<Segments> _segments;
		pipeline::Input<Blocks> _blocks;
		pipeline::Input<StackStore> _rawStackStore;
		pipeline::Output<LinearObjective> _objective;
	};
	
	void solve();
	
	pipeline::Value<Blocks> checkBlocks();
	
	void updateOutputs();
	
	pipeline::Input<PriorCostFunctionParameters> _priorCostFunctionParameters;
	pipeline::Input<SegmentationCostFunctionParameters> _segmentationCostFunctionParameters;
	pipeline::Input<Cores> _inCores;
	pipeline::Input<SegmentStore> _segmentStore;
	pipeline::Input<SliceStore> _sliceStore;
	pipeline::Input<StackStore> _rawImageStore;
	pipeline::Input<bool> _forceExplanation;
	pipeline::Input<unsigned int> _buffer;
	
	pipeline::Output<Blocks> _needBlocks;
	
	boost::shared_ptr<Blocks> _bufferedBlocks;
};

#endif //SOLUTION_GUARANTOR_H__
