#include <iostream>
#include <catmaid/persistence/postgresql/PostgreSqlSliceStore.h>
#include <util/exceptions.h>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/point.hpp>

util::ProgramOption optionProjectId(
		util::_long_name        = "project",
		util::_short_name       = "p",
		util::_description_text = "The Sopnet project ID.",
		util::_default_value	  = 3);

util::ProgramOption optionStackId(
		util::_long_name        = "stack",
		util::_short_name       = "s",
		util::_description_text = "The Sopnet raw stack ID.",
		util::_default_value	  = 2);

util::ProgramOption optionHost(
		util::_long_name        = "host",
		util::_short_name       = "m",
		util::_description_text = "The CATMAID host",
		util::_default_value	  = "neurocity.janelia.org/catsop");

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

int main(int argc, char** argv)
{
	try {
		// init command line parser
		util::ProgramOptions::init(argc, argv);

		std::string host = optionHost.as<std::string>();
		int project_id = optionProjectId.as<int>();
		int stack_id = optionStackId.as<int>();


		std::cout << "Testing PostgreSQL stores with host \"" << host <<
				"\", project ID " << project_id << " and stack ID " <<
				stack_id << std::endl;

		// init logger
		logger::LogManager::init();

		boost::shared_ptr<DjangoBlockManager> blockManager =
				DjangoBlockManager::getBlockManager(host, stack_id, project_id);

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
