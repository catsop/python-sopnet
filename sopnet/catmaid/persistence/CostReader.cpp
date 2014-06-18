#include "CostReader.h"
#include <limits>

CostReader::CostReader()
{
	registerInput(_store, "store");
	registerInput(_segments, "segments");
	registerInput(_defaultCost, "default cost", pipeline::Optional);
	registerOutput(_objective, "objective");
	registerOutput(_noCostSegments, "costless segments");
}

void CostReader::updateOutputs()
{
	pipeline::Value<LinearObjective> objective;
	pipeline::Value<Segments> noCostSegments = pipeline::Value<Segments>();
	
	double cost = _defaultCost.isSet() ? *_defaultCost : std::numeric_limits<double>::max();
	
	objective = _store->retrieveCost(_segments, cost, noCostSegments);

	_objective = new LinearObjective();
	_noCostSegments = new Segments();
	
	*_objective = *objective;
	*_noCostSegments = *noCostSegments;
}
