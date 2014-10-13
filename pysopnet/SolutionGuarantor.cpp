#include <pipeline/Value.h>
#include <pipeline/Process.h>
#include <catmaid/guarantors/SolutionGuarantor.h>
#include <catmaid/blocks/Cores.h>
#include <util/point3.hpp>
#include "SolutionGuarantor.h"
#include "logging.h"

namespace python {

Locations
SolutionGuarantor::fill(
		const util::point3<unsigned int>& request,
		const SolutionGuarantorParameters& parameters,
		const ProjectConfiguration& configuration) {

	LOG_USER(pylog) << "[SolutionGuarantor] fill called for core at " << request << std::endl;

	//boost::shared_ptr<BlockManager> blockManager  = createBlockManager(configuration);
	boost::shared_ptr<SliceStore>   sliceStore    = createSliceStore(configuration);
	boost::shared_ptr<SegmentStore> segmentStore  = createSegmentStore(configuration);

	// TODO: get them from somewhere else
	std::vector<double> featureWeights;
	featureWeights.push_back(0.00558637);
	featureWeights.push_back(-1.6678e-05);
	featureWeights.push_back(0.00204453);
	featureWeights.push_back(0.0711393);
	featureWeights.push_back(-0.00135737);
	featureWeights.push_back(3.35817);
	featureWeights.push_back(-0.000916876);
	featureWeights.push_back(-0.000957261);
	featureWeights.push_back(-0.00193582);
	featureWeights.push_back(-1.48732);
	featureWeights.push_back(-0.000234868);
	featureWeights.push_back(-4.21938);
	featureWeights.push_back(0.501363);
	featureWeights.push_back(0.0665533);
	featureWeights.push_back(-0.292248);
	featureWeights.push_back(0.0361189);
	featureWeights.push_back(0.0844144);
	featureWeights.push_back(-0.0316035);
	featureWeights.push_back(0.0127795);
	featureWeights.push_back(-0.00765338);
	featureWeights.push_back(-0.00558571);
	featureWeights.push_back(-0.0172858);
	featureWeights.push_back(0.00562492);
	featureWeights.push_back(-0.0109868);
	featureWeights.push_back(-0.00136111);
	featureWeights.push_back(-0.0227562);
	featureWeights.push_back(-0.0825309);
	featureWeights.push_back(-0.131062);
	featureWeights.push_back(-0.442795);
	featureWeights.push_back(0.354401);
	featureWeights.push_back(0.266398);
	featureWeights.push_back(1.46761);
	featureWeights.push_back(-0.743354);
	featureWeights.push_back(-0.281164);
	featureWeights.push_back(0.169887);
	featureWeights.push_back(0.262849);
	featureWeights.push_back(-0.0505789);
	featureWeights.push_back(0.00516085);
	featureWeights.push_back(0.0138543);
	featureWeights.push_back(-0.0102862);
	featureWeights.push_back(0.0080712);
	featureWeights.push_back(0.00012668);
	featureWeights.push_back(-0.0031432);
	featureWeights.push_back(0.00186596);
	featureWeights.push_back(0.00371999);
	featureWeights.push_back(-0.0688746);
	featureWeights.push_back(0.324525);
	featureWeights.push_back(0.79521);
	featureWeights.push_back(1.88847);
	featureWeights.push_back(2.09861);
	featureWeights.push_back(1.51523);
	featureWeights.push_back(0.394032);
	featureWeights.push_back(0.477188);
	featureWeights.push_back(-0.0952926);
	featureWeights.push_back(0.374847);
	featureWeights.push_back(0.253683);
	featureWeights.push_back(0.840265);
	featureWeights.push_back(-2.89614);
	featureWeights.push_back(4.2625e-10);

	// create the SolutionGuarantor process node
	::SolutionGuarantor solutionGuarantor(
			configuration,
			segmentStore,
			sliceStore,
			parameters.getCorePadding(),
			featureWeights);

	LOG_USER(pylog) << "[SolutionGuarantor] processing..." << std::endl;

	// find the core that corresponds to the request
	Core core(request.x, request.y, request.z);

	// let it do what it was build for
	Blocks missingBlocks = solutionGuarantor.guaranteeSolution(core);

	LOG_USER(pylog) << "[SolutionGuarantor] collecting missing segment blocks" << std::endl;

	// collect missing block locations
	Locations missing;
	foreach (const Block& block, missingBlocks)
		missing.push_back(util::point3<unsigned int>(block.x(), block.y(), block.z()));

	return missing;
}

} // namespace python

