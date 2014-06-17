#include "SolutionReader.h"

SolutionReader::SolutionReader()
{
	registerInput(_store, "store");
	registerInput(_core, "core");
	registerInput(_segments, "segments");
	registerOutput(_solution, "solution");
}

void
SolutionReader::updateOutputs()
{
	pipeline::Value<Solution> solution = _store->retrieveSolution(_segments, _core);

	_solution = new Solution();
	*_solution = *solution;
}

