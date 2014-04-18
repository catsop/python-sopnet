#ifndef END_EXTRACTOR_H__
#define END_EXTRACTOR_H__

#include <pipeline/all.h>
#include <sopnet/segments/Segments.h>
#include <sopnet/slices/Slices.h>

/**
	 * Guarantees that we have correct EndSegments at the section representing the upper bound
	 * of our sub stack. The SegmentGuarantor wouldn't necessarily have extracted these.
	 */
	class EndExtractor : public pipeline::SimpleProcessNode<>
	{
	public:
		EndExtractor();
		
	private:
		void updateOutputs();
		
		pipeline::Input<Segments> _eeSegments;
		pipeline::Input<Slices> _eeSlices;
		
		pipeline::Output<Segments> _allSegments;
	};
	

#endif //END_EXTRACTOR_H__