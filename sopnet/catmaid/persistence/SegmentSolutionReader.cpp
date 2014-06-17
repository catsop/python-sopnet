#include "SegmentSolutionReader.h"

SegmentSolutionReader::SegmentSolutionReader()
{
	registerInput(_store, "store");
	registerInput(_core, "core");
	registerInput(_segments, "segments");
	registerOutput(_solution, "solution");
}

void
SegmentSolutionReader::updateOutputs()
{
	pipeline::Value<Solution> solution = _store->retrieveSolution(_segments, _core);

	_solution = new Solution();
	*_solution = *solution;
}

