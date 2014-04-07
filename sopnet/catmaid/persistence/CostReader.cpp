#include "CostReader.h"
#include <limits>

CostReader::CostReader()
{
	registerInput(_store, "store");
	registerInput(_segments, "segments");
	registerInput(_defaultCost, "default cost", pipeline::Optional);
	registerOutput(_objective, "objective");
}

void CostReader::updateOutputs()
{
	pipeline::Value<LinearObjective> objective;
	
	double cost = _defaultCost ? *_defaultCost : std::numeric_limits<double>::max();
	
	objective = _store->retrieveCost(_segments, cost);
	
	*_objective = *objective;
}
