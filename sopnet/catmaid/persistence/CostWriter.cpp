#include "CostWriter.h"

CostWriter::CostWriter()
{
	registerInput(_store, "store");
	registerInput(_segments, "segments");
	registerInput(_objective, "objective");
}

unsigned int CostWriter::writeCosts()
{
	updateInputs();
	return _store->storeCost(_segments, _objective);
}

