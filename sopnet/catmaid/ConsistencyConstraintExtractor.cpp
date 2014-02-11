#include "ConsistencyConstraintExtractor.h"

#include <boost/make_shared.hpp>
#include <boost/unordered_set.hpp>

#include <deque>

#include <sopnet/inference/LinearConstraint.h>
#include <sopnet/slices/Slice.h>
#include <sopnet/slices/ComponentTreeConverter.h>
#include <sopnet/segments/Segment.h>
#include <pipeline/Value.h>
#include <util/Logger.h>
logger::LogChannel consistencyconstraintextractorlog("consistencyconstraintextractorlog",
													 "[ConsistencyConstraintExtractor] ");

typedef ComponentTree::Node Node;

ConsistencyConstraintExtractor::ConsistencyConstraintExtractor()
{
	registerInput(_slices, "slices");
	registerInput(_segments, "segments");
	registerInput(_forceExplanation, "force explanation");
	registerInput(_trees, "component trees");
	registerOutput(_linearConstraints, "linear constraints");
}

void ConsistencyConstraintExtractor::updateOutputs()
{
	LOG_DEBUG(consistencyconstraintextractorlog) << "Updating outputs" << std::endl;
	boost::shared_ptr<LinearConstraints> linearConstraints = boost::make_shared<LinearConstraints>();
	SectionSlices sliceMap;
	SectionSlices::const_iterator iter;
	_sliceSet.clear();
	_sliceSegments.clear();
	
	LOG_DEBUG(consistencyconstraintextractorlog) << "Mapping slices to segments" << std::endl;
	
	mapSliceSegments();

	LOG_DEBUG(consistencyconstraintextractorlog) << "Mapping slices to sections" << std::endl;
	
	foreach (boost::shared_ptr<Slice> slice, *_slices)
	{
		getOrCreate(slice->getSection(), sliceMap)->add(slice);
		_sliceSet.insert(*slice);
	}
	
	for (iter = sliceMap.begin(); iter != sliceMap.end(); ++iter)
	{
		LOG_DEBUG(consistencyconstraintextractorlog) << "Assembling constraints for section " <<
			iter->first << std::endl;
		boost::shared_ptr<LinearConstraints> sliceConstraints = 
			collectSliceConstraints(iter->first, iter->second);
		boost::shared_ptr<LinearConstraints> segmentConstraints = 
			assembleSegmentConstraints(sliceConstraints);
		linearConstraints->addAll(*segmentConstraints);
	}
	
	*_linearConstraints = *linearConstraints;
}

boost::shared_ptr<LinearConstraints>
ConsistencyConstraintExtractor::collectSliceConstraints(unsigned int section,
														const boost::shared_ptr<Slices>& slices)
{
	boost::shared_ptr<ComponentTreeConverter> converter =
			boost::make_shared<ComponentTreeConverter>(section);
	boost::shared_ptr<ComponentTree> tree = _trees->getTree(section);
	boost::shared_ptr<LinearConstraints> sliceConstraints = boost::make_shared<LinearConstraints>();
	std::deque<unsigned int> path;
	boost::unordered_map<ConnectedComponent, boost::shared_ptr<Slice> > componentSliceMap;
	
	foreach (boost::shared_ptr<Slice> slice, *slices)
	{
		componentSliceMap[*(slice->getComponent())] = slice;
	}
	
	foreach (boost::shared_ptr<ComponentTree::Node> node, tree->getRoot()->getChildren())
	{
		path.clear();
		addConstraints(node, sliceConstraints, path, componentSliceMap);
	}
	
	LOG_DEBUG(consistencyconstraintextractorlog) << "Collected " << sliceConstraints->size() <<
		" constraints" << std::endl;
	
	return sliceConstraints;
}

void
ConsistencyConstraintExtractor::addConstraints(const boost::shared_ptr<ComponentTree::Node>& node,
								const boost::shared_ptr<LinearConstraints>& constraints,
								std::deque<unsigned int>& path,
								boost::unordered_map<ConnectedComponent, boost::shared_ptr<Slice> >&
																	componentSliceMap)
{
	unsigned int id = componentSliceMap[*node->getComponent()]->getId();
	path.push_back(id);
	
	if (node->getChildren().size() == 0)
	{
		// Cribbed from ComponentTreeConverter::addConstraints
		LinearConstraint constraint;
		foreach (unsigned int id, path)
		{
			constraint.setCoefficient(id, 1);
		}
		constraint.setValue(1);
		constraints->add(constraint);
	}
	else
	{
		foreach (boost::shared_ptr<Node> childNode, node->getChildren())
		{
			addConstraints(childNode, constraints, path, componentSliceMap);
		}
	}
	
	if (*(path.end() - 1) != id)
	{
		LOG_ERROR(consistencyconstraintextractorlog) << "ID at end: " << *(path.end() - 1) <<
			" this id: " << id << std::endl;
	}
	
	path.pop_back();
}


boost::shared_ptr<LinearConstraints>
ConsistencyConstraintExtractor::assembleSegmentConstraints(
	const boost::shared_ptr<LinearConstraints>& sliceConstraints)
{
	LOG_DEBUG(consistencyconstraintextractorlog) << "Assembling segment constraints" << std::endl;
	boost::shared_ptr<LinearConstraints> segmentConstraints = 
		boost::make_shared<LinearConstraints>();
	//Code here lifted from SegmentExtractor::assembleLinearConstraints.
	//It would be a good idea to merge these functions to reduce redundant code.
	foreach (const LinearConstraint& sliceConstraint, *sliceConstraints)
	{
		LinearConstraint constraint;
		
		// for each slice in the constraint
		typedef std::map<unsigned int, double>::value_type pair_t;
		foreach (const pair_t& pair, sliceConstraint.getCoefficients()) {

			unsigned int sliceId = pair.first;

			// for all the segments that involve this slice
			const std::vector<unsigned int> segmentIds = _sliceSegments[sliceId];

			foreach (unsigned int segmentId, segmentIds)
				constraint.setCoefficient(segmentId, 1.0);
		}

		if (_forceExplanation && ! *_forceExplanation)
		{
			constraint.setRelation(LessEqual);
		}
		else
		{
			constraint.setRelation(Equal);
		}

		constraint.setValue(1);
		
		segmentConstraints->add(constraint);
	}
	LOG_DEBUG(consistencyconstraintextractorlog) << "Done." << std::endl;
	return segmentConstraints;
}


std::vector<boost::shared_ptr<ComponentTree> >
ConsistencyConstraintExtractor::createComponentTrees(const boost::shared_ptr<Slices>& slices)
{
	std::vector<boost::shared_ptr<ComponentTree> > trees;
	std::vector<boost::shared_ptr<ConnectedComponent> > components;
	std::vector<boost::shared_ptr<Node> > rootNodes;
	
	foreach (boost::shared_ptr<Slice> slice, *slices)
	{
		components.push_back(slice->getComponent());
	}
	
	// Sort connected components by size, in descending order.
	std::sort(components.begin(), components.end(),
			  &ConsistencyConstraintExtractor::compareConnectedComponents);
	
	// Iterate over the Components, inserting into a tree of ComponentTree::Node
	foreach (boost::shared_ptr<ConnectedComponent> component, components)
	{
		boost::shared_ptr<Node> parent = boost::shared_ptr<Node>();
		boost::shared_ptr<Node> node = boost::make_shared<Node>(component);
		
		// We're not necessarily guaranteed to have a single node that overlaps all others.
		// Find the first node in rootNodes that intersects component
		foreach (boost::shared_ptr<Node> node, rootNodes)
		{
			if (node->getComponent()->intersects(*component))
			{
				parent = node;
				break;
			}
		}
		
		// If we didn't find a parent, create a new Node for this component and put it in rootNodes
		if (!parent)
		{
			
			rootNodes.push_back(node);
		}
		else
		{
			// Otherwise, descend into the tree.
			boost::shared_ptr<Node> currParent = parent;
			boost::shared_ptr<Node> nextParent = parent;
			boost::shared_ptr<Node> nullNode = boost::shared_ptr<Node>();
			
			while (nextParent)
			{
				currParent = nextParent;
				nextParent = nullNode;
				
				// We know that currParent intersects component
				// Iterate over the children. The first one that intersects
				// component, if it exists, will become nextParent.
				// If it doesn't exist, we'll make component a child of currParent.
				foreach (boost::shared_ptr<Node> child, currParent->getChildren())
				{
					if (child->getComponent()->intersects(*component))
					{
						nextParent = child;
						break;
					}
				}
			}
			
			currParent->addChild(node);
			node->setParent(currParent);
		}
	}

	
	boost::shared_ptr<ComponentTree> tree = boost::make_shared<ComponentTree>();
	boost::shared_ptr<ConnectedComponent> empty = boost::make_shared<ConnectedComponent>();
	boost::shared_ptr<Node> node = boost::make_shared<Node>(empty);
	tree->setRoot(node);
	
	foreach (boost::shared_ptr<Node> rootNode, rootNodes)
	{
		rootNode->setParent(node);
		node->addChild(rootNode);
	}
	
	trees.push_back(tree);
	
	return trees;
}

boost::shared_ptr<LinearConstraints>
ConsistencyConstraintExtractor::mapSliceIds(
	const boost::shared_ptr<LinearConstraints>& constraints,
	boost::unordered_map<unsigned int, unsigned int>& idMap)
{
	boost::shared_ptr<LinearConstraints> mappedConstraints =
		boost::make_shared<LinearConstraints>();
		
	LOG_DEBUG(consistencyconstraintextractorlog) << "Mapping Slice IDs in LinearConstraints" << std::endl;
		
	foreach (const LinearConstraint constraint, *constraints)
	{
		boost::shared_ptr<LinearConstraint> mappedConstraint = 
			boost::make_shared<LinearConstraint>();
		std::map<unsigned int, double> coefs = constraint.getCoefficients();
		std::map<unsigned int, double>::iterator iter;
		
		mappedConstraint->setRelation(constraint.getRelation());
		mappedConstraint->setValue(constraint.getValue());
		
		for (iter = coefs.begin(); iter != coefs.end(); ++iter)
		{
			mappedConstraint->setCoefficient(idMap[iter->first], iter->second);
		}
		
		mappedConstraints->add(*mappedConstraint);
	}
	
	LOG_DEBUG(consistencyconstraintextractorlog) << "Done. Returning " << mappedConstraints->size()
		<< " constraints" << std::endl;
	
	return mappedConstraints;
}

boost::shared_ptr<Slices>
ConsistencyConstraintExtractor::getOrCreate(unsigned int section, SectionSlices& sliceMap)
{
	if (sliceMap.count(section))
	{
		return sliceMap[section];
	}
	else
	{
		boost::shared_ptr<Slices> slices = boost::make_shared<Slices>();
		sliceMap[section] = slices;
		return slices;
	}
}

void ConsistencyConstraintExtractor::mapSliceSegments()
{
	foreach (boost::shared_ptr<EndSegment> end, _segments->getEnds())
	{
		if (end->getDirection() == Right)
		{
			_sliceSegments[end->getSlice()->getId()].push_back(end->getId());
		}
	}
	
	foreach (boost::shared_ptr<ContinuationSegment> continuation, _segments->getContinuations())
	{
		_sliceSegments[continuation->getSourceSlice()->getId()].push_back(continuation->getId());
	}
	
	foreach (boost::shared_ptr<BranchSegment> branch, _segments->getBranches())
	{
		if (branch->getDirection() == Left)
		{
			_sliceSegments[branch->getTargetSlice1()->getId()].push_back(branch->getId());
			_sliceSegments[branch->getTargetSlice2()->getId()].push_back(branch->getId());
		}
		else
		{
			_sliceSegments[branch->getSourceSlice()->getId()].push_back(branch->getId());
		}
	}
}

bool
ConsistencyConstraintExtractor::compareConnectedComponents(
	const boost::shared_ptr<ConnectedComponent>& comp1,
	const boost::shared_ptr<ConnectedComponent>& comp2)
{
	return comp1->getSize() > comp2->getSize();
}

