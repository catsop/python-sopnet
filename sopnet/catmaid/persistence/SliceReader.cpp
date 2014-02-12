#include "SliceReader.h"
#include <sopnet/block/Block.h>


#include <util/Logger.h>
logger::LogChannel slicereaderlog("slicereaderlog", "[SliceReader] ");

SliceReader::SliceReader()
{
	registerInput(_blocks, "blocks");
	registerInput(_store, "store");
	
	registerOutput(_slices, "slices");
	registerOutput(_conflictSets, "conflict sets");
}

void SliceReader::updateOutputs()
{
	pipeline::Value<Slices> slices;
	pipeline::Value<ConflictSets> conflict;

	LOG_DEBUG(slicereaderlog) << "Retrieving block slices" << std::endl;

	slices = _store->retrieveSlices(_blocks);
	conflict = _store->retrieveConflictSets(slices);
	
	*_slices = *slices;
	*_conflictSets = *conflict;

	LOG_DEBUG(slicereaderlog) << "Done." << std::endl;
}
