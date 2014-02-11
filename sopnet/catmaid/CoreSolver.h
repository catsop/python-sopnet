#ifndef CORE_SOLVER_H__
#define CORE_SOLVER_H__

#include <boost/shared_ptr.hpp>
#include <pipeline/all.h>
#include <imageprocessing/io/ImageBlockFactory.h>
#include <imageprocessing/io/ImageBlockStackReader.h>
#include <sopnet/inference/PriorCostFunctionParameters.h>
#include <sopnet/inference/ProblemAssembler.h>
#include <sopnet/inference/LinearSolver.h>
#include <sopnet/inference/Reconstructor.h>
#include <sopnet/inference/PriorCostFunction.h>
#include <sopnet/inference/ProblemAssembler.h>
#include <sopnet/inference/RandomForestCostFunction.h>
#include <sopnet/inference/ObjectiveGenerator.h>
#include <sopnet/inference/SegmentationCostFunctionParameters.h>
#include <sopnet/inference/SegmentationCostFunction.h>
#include <sopnet/inference/io/RandomForestHdf5Reader.h>
#include <sopnet/neurons/NeuronExtractor.h>
#include <sopnet/block/Blocks.h>
#include <sopnet/block/Box.h>
#include <sopnet/block/BlockManager.h>
#include <sopnet/features/SegmentFeaturesExtractor.h>
#include <catmaid/persistence/SegmentStore.h>
#include <catmaid/persistence/SliceStore.h>
#include <catmaid/persistence/SegmentReader.h>
#include <catmaid/persistence/SliceReader.h>

class CoreSolver : public pipeline::SimpleProcessNode<>
{
public:
	CoreSolver();
	
	boost::shared_ptr<ProblemAssembler> getProblemAssembler()
	{
		return _problemAssembler;
	}
	
private:
	void updateOutputs();
	boost::shared_ptr<Blocks> computeBound();
	
	pipeline::Input<PriorCostFunctionParameters> _priorCostFunctionParameters;
	pipeline::Input<SegmentationCostFunctionParameters> _segmentationCostFunctionParameters;
	pipeline::Input<Blocks> _blocks;
	pipeline::Input<SegmentStore> _segmentStore;
	pipeline::Input<SliceStore> _sliceStore;
	pipeline::Input<ImageBlockFactory> _rawImageFactory;
	pipeline::Input<ImageBlockFactory> _membraneFactory;
	pipeline::Input<bool> _forceExplanation;
	
	pipeline::Output<SegmentTrees> _neurons;
	
	boost::shared_ptr<ProblemAssembler> _problemAssembler;
	//boost::shared_ptr<ComponentTreeExtractor> _componentTreeExtractor;
	boost::shared_ptr<Reconstructor> _reconstructor;
	boost::shared_ptr<LinearSolver> _linearSolver;
	boost::shared_ptr<NeuronExtractor> _neuronExtractor;
	boost::shared_ptr<SegmentReader> _segmentReader;
	boost::shared_ptr<SliceReader> _sliceReader;
	boost::shared_ptr<PriorCostFunction> _priorCostFunction;
	boost::shared_ptr<SegmentationCostFunction> _segmentationCostFunction;
	boost::shared_ptr<RandomForestHdf5Reader> _randomForestHDF5Reader;
	boost::shared_ptr<ImageBlockStackReader> _rawImageStackReader;
	boost::shared_ptr<ImageBlockStackReader> _membraneStackReader;
	boost::shared_ptr<RandomForestCostFunction> _randomForestCostFunction;
	boost::shared_ptr<SegmentFeaturesExtractor> _segmentFeaturesExtractor;
	boost::shared_ptr<ObjectiveGenerator> _objectiveGenerator;

};

#endif //CORE_SOLVER_H__
