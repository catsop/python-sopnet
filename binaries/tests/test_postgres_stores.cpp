#include <iostream>
#include <catmaid/persistence/postgresql/PostgreSqlSliceStore.h>
#include <util/exceptions.h>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/point.hpp>

boost::shared_ptr<Slice>
createSlice() {

	boost::shared_ptr<ConnectedComponent::pixel_list_type> pixelList =
			boost::make_shared<ConnectedComponent::pixel_list_type>();

	pixelList->push_back(util::point<unsigned int>(0,0));
	pixelList->push_back(util::point<unsigned int>(1,1));

	boost::shared_ptr<Image> source = boost::make_shared<Image>(10,10);

	boost::shared_ptr<ConnectedComponent> cc = boost::make_shared<ConnectedComponent>(
			source,
			0,
			pixelList,
			0,
			1);

	return boost::make_shared<Slice>(0, 0, cc);
}

int main(int argc, char** argv) {

	std::string host = argc > 1 ? argv[1] : "neurocity.janelia.org/catsop";
	int project_id = argc > 2 ? atoi(argv[2]) : 3;
	int stack_id = argc > 3 ? atoi(argv[3]) : 2;

	std::cout << "Testing PostgreSQL stores with host \"" << host <<
			"\", project ID " << project_id << " and stack ID " <<
			stack_id << std::endl;

	try {

		// init command line parser
		util::ProgramOptions::init(argc, argv);

		// init logger
		logger::LogManager::init();

		boost::shared_ptr<DjangoBlockManager> blockManager =
				DjangoBlockManager::getBlockManager(host, project_id, stack_id);

		PostgreSqlSliceStore store(blockManager);

		boost::shared_ptr<Block> block = blockManager->blockAtLocation(util::point3<unsigned int>(0, 0, 0));
		boost::shared_ptr<Slice> slice = createSlice();

		Slices slices = Slices();
		slices.add(slice);

		store.associateSlicesToBlock(slices, *block);

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
	}
}
