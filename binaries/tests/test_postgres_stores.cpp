#include <iostream>
#include <catmaid/persistence/postgresql/PostgreSqlSliceStore.h>
#include <util/exceptions.h>
#include <util/Logger.h>
#include <util/ProgramOptions.h>

boost::shared_ptr<Slice>
createSlice() {

	boost::shared_ptr<ConnectedComponent::pixel_list_type> pixelList =
			boost::make_shared<ConnectedComponent::pixel_list_type>();

	boost::shared_ptr<ConnectedComponent> cc = boost::make_shared<ConnectedComponent>(
			boost::shared_ptr<Image>(),
			0,
			pixelList,
			0,
			0);

	return boost::make_shared<Slice>(0, 0, cc);
}

int main(int argc, char** argv) {

	std::cout << "Testing PostgreSQL stores" << std::endl;

	try {

		// init command line parser
		util::ProgramOptions::init(argc, argv);

		// init logger
		logger::LogManager::init();

		boost::shared_ptr<DjangoBlockManager> blockManager =
				DjangoBlockManager::getBlockManager("neurocity.janelia.org/catsop", 3, 2);

		PostgreSqlSliceStore store(blockManager);

		boost::shared_ptr<Block> block = blockManager->blockAtLocation(util::point3<unsigned int>(0, 0, 0));
		boost::shared_ptr<Slice> slice = createSlice();

		boost::shared_ptr<Slices> slices = boost::make_shared<Slices>();
		slices->add(slice);

		store.associate(slices, block);

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
	}
}
