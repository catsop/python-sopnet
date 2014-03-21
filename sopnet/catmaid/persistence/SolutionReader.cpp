#include "SolutionReader.h"

SolutionReader::SolutionReader()
{
	registerInput(_store, "store");
	registerInput(_segments, "segments");
	registerOutput(_solution, "solution");
}

void
SolutionReader::updateOutputs()
{
	pipeline::Value<Solution> solution = _store->retrieveSolution(_segments);
	*_solution = *solution;
}

