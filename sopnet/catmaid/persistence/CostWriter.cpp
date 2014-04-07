#include "CostWriter.h"

CostWriter::CostWriter()
{
	registerInput(_store, "store");
	registerInput(_segments, "segments");
	registerInput(_objective, "objective");
}

void CostWriter::writeCosts()
{
	updateInputs();
	_store->storeCost(_segments, _objective);
}

