#ifndef DJANGO_SEGMENT_STORE_H__
#define DJANGO_SEGMENT_STORE_H__

#include <catmaid/persistence/SegmentStore.h>
#include "DjangoSliceStore.h"

class DjangoSegmentStore : public SegmentStore
{
public:
	DjangoSegmentStore(boost::shared_ptr<DjangoSliceStore> sliceStore);
	
	void associate(pipeline::Value<Segments> segments, pipeline::Value<Block> block);

	pipeline::Value<Segments> retrieveSegments(pipeline::Value<Blocks> blocks);

	pipeline::Value<Blocks> getAssociatedBlocks(pipeline::Value<Segment> segment);

	int storeFeatures(pipeline::Value<Features> features);

	pipeline::Value<SegmentFeaturesMap> retrieveFeatures(pipeline::Value<Segments> segments);

	std::vector<std::string> getFeatureNames();

	unsigned int storeCost(pipeline::Value<Segments> segments,
							   pipeline::Value<LinearObjective> objective);

	pipeline::Value<LinearObjective> retrieveCost(pipeline::Value<Segments> segments,
														  double defaultCost,
														  pipeline::Value<Segments> segmentsNF);

	unsigned int storeSolution(pipeline::Value<Segments> segments,
									   pipeline::Value<Core> core,
									   pipeline::Value<Solution> solution,
									   std::vector<unsigned int> indices);

	pipeline::Value<Solution> retrieveSolution(pipeline::Value<Segments> segments,
													   pipeline::Value<Core> core);

	void dumpStore();

private:
	const boost::shared_ptr<DjangoSliceStore> _sliceStore;
};


#endif //DJANGO_SEGMENT_STORE_H__