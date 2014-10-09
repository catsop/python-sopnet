#include <iostream>
#include <catmaid/ProjectConfiguration.h>
#include <catmaid/persistence/postgresql/PostgreSqlSegmentStore.h>
#include <catmaid/persistence/postgresql/PostgreSqlSliceStore.h>
#include <util/exceptions.h>
#include <util/rect.hpp>
#include <util/Logger.h>
#include <util/ProgramOptions.h>
#include <util/point.hpp>
#include <sopnet/slices/ConflictSets.h>

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

util::ProgramOption optionComponentDir(
		util::_long_name        = "compdir",
		util::_short_name       = "c",
		util::_description_text = "Component storage directory",
		util::_default_value	  = "/tmp/catsop");

util::ProgramOption optionPGHost(
		util::_long_name        = "pghost",
		util::_short_name       = "H",
		util::_description_text = "The PostgreSQL host",
		util::_default_value	  = "");

util::ProgramOption optionPGUser(
		util::_long_name        = "pguser",
		util::_short_name       = "U",
		util::_description_text = "The PostgreSQL user",
		util::_default_value	  = "catsop_user");

util::ProgramOption optionPGPassword(
		util::_long_name        = "pgpassword",
		util::_short_name       = "P",
		util::_description_text = "The PostgreSQL password",
		util::_default_value	  = "catsop_janelia_test");

util::ProgramOption optionPGDatabase(
		util::_long_name        = "pgdatabase",
		util::_short_name       = "D",
		util::_description_text = "The PostgreSQL database",
		util::_default_value	  = "catsop");

boost::shared_ptr<Slice>
createSlice(unsigned int sourceSize, unsigned int pixelEntry) {

	boost::shared_ptr<ConnectedComponent::pixel_list_type> pixelList =
			boost::make_shared<ConnectedComponent::pixel_list_type>();

	pixelList->push_back(util::point<unsigned int>(pixelEntry, pixelEntry));
	pixelList->push_back(util::point<unsigned int>(pixelEntry + 1, pixelEntry + 1));

	boost::shared_ptr<Image> source = boost::make_shared<Image>(sourceSize, sourceSize);

	boost::shared_ptr<ConnectedComponent> cc = boost::make_shared<ConnectedComponent>(
			source,
			0,
			pixelList,
			0,
			2);

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
		std::string comp_dir = optionComponentDir.as<std::string>();
		std::string pg_host = optionPGHost.as<std::string>();
		std::string pg_user = optionPGUser.as<std::string>();
		std::string pg_pass = optionPGPassword.as<std::string>();
		std::string pg_dbase = optionPGDatabase.as<std::string>();


		std::cout << "Testing PostgreSQL stores with host \"" << host <<
				"\", project ID " << project_id << " and stack ID " <<
				stack_id << std::endl;

		// init logger
		logger::LogManager::init();

		// create new project configuration
		ProjectConfiguration pc;
		pc.setBackendType(ProjectConfiguration::PostgreSql);
		pc.setCatmaidHost(host);
		pc.setCatmaidProjectId(project_id);
		pc.setCatmaidRawStackId(stack_id);
		pc.setComponentDirectory(comp_dir);
		pc.setPostgreSqlHost(pg_host);
		pc.setPostgreSqlUser(pg_user);
		pc.setPostgreSqlPassword(pg_pass);
		pc.setPostgreSqlDatabase(pg_dbase);

		PostgreSqlSliceStore sliceStore(pc);

		// Add first set of slices
		boost::shared_ptr<Slice> slice1 = createSlice(10, 0);
		boost::shared_ptr<Slice> slice2 = createSlice(10, 1);
		boost::shared_ptr<Slice> slice3 = createSlice(10, 2);

		Slices slices = Slices();
		slices.add(slice1);
		slices.add(slice2);
		slices.add(slice3);

		Block block(0, 0, 0);
		sliceStore.associateSlicesToBlock(slices, block);

		Blocks blocks;
		blocks.add(block);
		Blocks missingBlocks;

		boost::shared_ptr<Slices> retrievedSlices =
				sliceStore.getSlicesByBlocks(blocks, missingBlocks);

		// Create conflict set where each slice
		ConflictSet conflictSet1;
		conflictSet1.addSlice(slice1->hashValue());
		conflictSet1.addSlice(slice2->hashValue());
		conflictSet1.addSlice(slice3->hashValue());

		ConflictSets conflictSets;
		conflictSets.add(conflictSet1);

		sliceStore.associateConflictSetsToBlock(conflictSets, block);
		boost::shared_ptr<ConflictSets> retrievedConflictSets =
				sliceStore.getConflictSetsByBlocks(blocks, missingBlocks);
		foreach (const ConflictSet& cs, *retrievedConflictSets) {
			std::cout << "ConflictSet hash: " << hash_value(cs);

			foreach (const SliceHash& sh, cs.getSlices()) {
				std::cout << " Slice hash: " << sh;
			}

			std::cout << std::endl;
		}

		PostgreSqlSegmentStore segmentStore(pc);
		util::rect<unsigned int> segmentBounds(0, 0, 0, 0);
		util::point<double> segmentCenter(0.0, 0.0);
		std::vector<double> segmentFeatures;
		segmentFeatures.push_back(0.0);
		segmentFeatures.push_back(1.0);
		segmentFeatures.push_back(2.0);
		SegmentDescription segment(0, segmentBounds, segmentCenter);
		segment.addLeftSlice(slice1->hashValue());
		segment.addRightSlice(slice2->hashValue());
		segment.setFeatures(segmentFeatures);

		boost::shared_ptr<SegmentDescriptions> segments = boost::make_shared<SegmentDescriptions>();
		segments->add(segment);

		segmentStore.associateSegmentsToBlock(*segments, block);

		boost::shared_ptr<SegmentDescriptions> retrievedSegments =
				segmentStore.getSegmentsByBlocks(blocks, missingBlocks);

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
	}
}
