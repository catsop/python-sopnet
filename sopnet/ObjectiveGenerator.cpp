#include <util/foreach.h>
#include "ObjectiveGenerator.h"

static logger::LogChannel objectivegeneratorlog("objectivegeneratorlog", "[ObjectiveGenerator] ");

ObjectiveGenerator::ObjectiveGenerator() {

	registerInput(_segments, "segments");
	registerInput(_costFunction, "cost function");
	registerInput(_segmentIdsToVariables, "segment ids map");
	registerOutput(_objective, "objective");
}

void
ObjectiveGenerator::updateOutputs() {

	updateObjective();
}

void
ObjectiveGenerator::updateObjective() {

	LOG_DEBUG(objectivegeneratorlog) << "updating the objective" << std::endl;

	// we have as many linear coefficients as segments
	_objective->resize(_segments->size());

	// set the coefficient for each segment
	foreach (boost::shared_ptr<EndSegment> segment, _segments->getEnds())
		setCosts(*segment);

	foreach (boost::shared_ptr<ContinuationSegment> segment, _segments->getContinuations())
		setCosts(*segment);

	foreach (boost::shared_ptr<BranchSegment> segment, _segments->getBranches())
		setCosts(*segment);
}

template <typename SegmentType>
void
ObjectiveGenerator::setCosts(const SegmentType& segment) {

	unsigned int numVariable = (*_segmentIdsToVariables)[segment.getId()];

	LOG_ALL(objectivegeneratorlog) << "setting variable " << numVariable << " to " << (*_costFunction)(segment) << std::endl;

	_objective->setCoefficient(numVariable, (*_costFunction)(segment));
}