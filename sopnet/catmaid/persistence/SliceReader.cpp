#include "SliceReader.h"
#include <sopnet/block/Block.h>


#include <util/Logger.h>
logger::LogChannel slicereaderlog("slicereaderlog", "[SliceReader] ");

SliceReader::SliceReader()
{
	registerInput(_blocks, "blocks");
	registerInput(_store, "store");
	registerInput(_blockManager, "block manager");
	
	registerOutput(_slices, "slices");
	registerOutput(_conflictSets, "conflict sets");
}

void
SliceReader::addUnique(const boost::shared_ptr<Slices>& inSlices,
					   const boost::shared_ptr<Slices>& recvSlices,
					   boost::unordered::unordered_set<Slice>& set)
{
	foreach (const boost::shared_ptr<Slice>& slice, *inSlices)
	{
		if (!set.count(*slice))
		{
			set.insert(*slice);
			recvSlices->add(slice);
		}
	}
}

void SliceReader::updateOutputs()
{
	pipeline::Value<Slices> slices;

	LOG_DEBUG(slicereaderlog) << "Retrieving block slices" << std::endl;

	slices = _store->retrieveSlices(_blocks);
	*_slices = *slices;

	LOG_DEBUG(slicereaderlog) << "Done." << std::endl;	
}
