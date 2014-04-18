#include <util/Logger.h>

#include <sopnet/segments/SegmentSet.h>

#include "EndExtractor.h"

logger::LogChannel endextractorlog("endextractorlog", "[EndExtractor] ");

EndExtractor::EndExtractor()
{
	registerInput(_eeSegments, "segments");
	registerInput(_eeSlices, "slices");
	registerOutput(_allSegments, "segments");
}

void EndExtractor::updateOutputs()
{
	unsigned int z = 0;
	SegmentSet segmentSet;
	boost::shared_ptr<Segments> outputSegments = boost::make_shared<Segments>();
	
	LOG_DEBUG(endextractorlog) << "End extractor recieved " << _eeSegments->size() <<
		" segments" << std::endl;
	
	foreach (boost::shared_ptr<Slice> slice, *_eeSlices)
	{
		if (slice->getSection() > z)
		{
			z = slice->getSection();
		}
	}
	
	LOG_DEBUG(endextractorlog) << " End extractor: max z is " << z << std::endl;

	foreach (boost::shared_ptr<Segment> segment, _eeSegments->getSegments())
	{
		segmentSet.add(segment);
	}

	
	foreach (boost::shared_ptr<Slice> slice, *_eeSlices)
	{
		if (slice->getSection() == z)
		{
			int begSize = segmentSet.size();
			boost::shared_ptr<EndSegment> rightEnd =
				boost::make_shared<EndSegment>(Segment::getNextSegmentId(),
											   Left,
											   slice);
			boost::shared_ptr<EndSegment> leftEnd =
				boost::make_shared<EndSegment>(Segment::getNextSegmentId(),
											   Right,
											   slice);
			
			segmentSet.add(rightEnd);
			segmentSet.add(leftEnd);
			
			LOG_ALL(endextractorlog) << "Added " << (segmentSet.size() - begSize) << " segments" << std::endl;
		}
	}
	
	
	foreach (boost::shared_ptr<Segment> segment, segmentSet)
	{
		outputSegments->add(segment);
	}
	
	LOG_DEBUG(endextractorlog) << "End extractor returning " << outputSegments->size() <<
		" segments" << std::endl;
	
	*_allSegments = *outputSegments;
}