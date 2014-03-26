#include "SolutionWriter.h"

SolutionWriter::SolutionWriter()
{
	registerInput(_segments, "segments");
	registerInput(_cores, "cores");
	registerInput(_solution, "solution");
	registerInput(_store, "store");
}

void
SolutionWriter::writeSolution()
{
	updateInputs();
	
	foreach (boost::shared_ptr<Core> core, *_cores)
	{
		pipeline::Value<Core> valueCore(*core);
		pipeline::Value<Segments> subSegments;
		std::vector<unsigned int> idmap;
		util::rect<unsigned int> coreRectUI = *core;
		util::rect<int> coreRectI = static_cast<util::rect<int> >(coreRectUI);
		unsigned int i = 0;
		
		foreach (boost::shared_ptr<Segment> segment, _segments->getSegments())
		{
			if (segmentOverlapsCore(segment, coreRectI))
			{
				idmap.push_back(i);
				subSegments->add(segment);
			}
			
			++i;
		}
		
		_store->storeSolution(subSegments, valueCore, _solution, idmap);
	}
}

bool
SolutionWriter::segmentOverlapsCore(boost::shared_ptr<Segment> segment,
									const util::rect<int>& coreRect)
{
	foreach (boost::shared_ptr<Slice> slice, segment->getSlices())
	{
		if (coreRect.intersects(slice->getComponent()->getBoundingBox()))
		{
			return true;
		}
	}
	
	return false;
}
