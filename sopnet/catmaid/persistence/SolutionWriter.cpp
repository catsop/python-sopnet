#include "SolutionWriter.h"

SolutionWriter::SolutionWriter()
{
	registerInput(_segments, "segments");
	registerInput(_solution, "solution");
	registerInput(_store, "store");
}

void
SolutionWriter::writeSolution()
{
	updateInputs();
	
	_store->storeSolution(_segments, _solution);
}
