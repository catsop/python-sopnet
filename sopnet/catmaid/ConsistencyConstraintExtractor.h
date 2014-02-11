#ifndef CONSISTENCY_CONSTRAINT_EXTRACTOR_H__
#define CONSISTENCY_CONSTRAINT_EXTRACTOR_H__

#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#include <pipeline/all.h>
#include <sopnet/slices/Slices.h>
#include <sopnet/segments/Segments.h>
#include <sopnet/inference/LinearConstraints.h>
#include <imageprocessing/ComponentTree.h>
#include <imageprocessing/ComponentTrees.h>

class ConsistencyConstraintExtractor : public pipeline::SimpleProcessNode<>
{
	typedef boost::unordered_map<unsigned int, boost::shared_ptr<Slices> > SectionSlices;
public:
    ConsistencyConstraintExtractor();
	
private:
	void updateOutputs();
	
	std::vector<boost::shared_ptr<ComponentTree> > createComponentTrees(
		const boost::shared_ptr<Slices>& slices);
	
	boost::shared_ptr<LinearConstraints> mapSliceIds(
		const boost::shared_ptr<LinearConstraints>& constraints,
		boost::unordered_map<unsigned int, unsigned int>& idMap);
	
	void mapSliceSegments();
	
	boost::shared_ptr<Slices> getOrCreate(unsigned int section, SectionSlices& sliceMap);
	
	boost::shared_ptr<LinearConstraints> collectSliceConstraints(unsigned int section,
														const boost::shared_ptr<Slices>& slices);
	
	boost::shared_ptr<LinearConstraints> assembleSegmentConstraints(
		const boost::shared_ptr<LinearConstraints>& sliceConstraints);
	
	void addConstraints(const boost::shared_ptr<ComponentTree::Node>& node,
						const boost::shared_ptr<LinearConstraints>& constraints,
						std::deque<unsigned int>& path,
						boost::unordered_map<ConnectedComponent, boost::shared_ptr<Slice> >&
																				componentSliceMap);
	
	static bool compareConnectedComponents(const boost::shared_ptr<ConnectedComponent>& comp1,
										   const boost::shared_ptr<ConnectedComponent>& comp2);

	pipeline::Input<Slices> _slices;
	pipeline::Input<Segments> _segments;
	pipeline::Input<bool> _forceExplanation;
	pipeline::Input<ComponentTrees> _trees;
	pipeline::Output<LinearConstraints> _linearConstraints;
	
	boost::unordered_set<Slice> _sliceSet;
	boost::unordered_map<unsigned int, std::vector<unsigned int> > _sliceSegments;
	
};

#endif //CONSISTENCY_CONSTRAINT_EXTRACTOR_H__