#include <boost/lexical_cast.hpp>
#include "PostgreSqlProjectConfigurationStore.h"
#include "PostgreSqlUtils.h"

PostgreSqlProjectConfigurationStore::PostgreSqlProjectConfigurationStore(const ProjectConfiguration& config) {

	_pgConnection = PostgreSqlUtils::getConnection(
			config.getPostgreSqlHost(),
			config.getPostgreSqlPort(),
			config.getPostgreSqlDatabase(),
			config.getPostgreSqlUser(),
			config.getPostgreSqlPassword());
	std::ostringstream q;
	q << "SET search_path TO segstack_"
	  << config.getCatmaidStack(Raw).segmentationId
	  << ",public;";
	PQsendQuery(_pgConnection, q.str().c_str());
}

PostgreSqlProjectConfigurationStore::~PostgreSqlProjectConfigurationStore() {

	if (_pgConnection != 0) {
		PostgreSqlUtils::waitForAsyncQuery(_pgConnection);
		PQfinish(_pgConnection);
	}
}

void
PostgreSqlProjectConfigurationStore::fill(ProjectConfiguration& config) {

	// fill stack descriptions for raw and membrane

	StackDescription rawStackDescription = config.getCatmaidStack(Raw);
	StackDescription memStackDescription = config.getCatmaidStack(Membrane);

	fill(rawStackDescription, Raw);
	fill(memStackDescription, Membrane);

	// fill project configuration

	std::stringstream q;
	q <<
			"SELECT num_x, num_y, num_z, block_dim_x, block_dim_y, block_dim_z, core_dim_x, core_dim_y, core_dim_z, scale "
			"FROM segmentation_block_info "
			"WHERE configuration_id=" << rawStackDescription.segmentationId;
	PGresult* result = PQexec(_pgConnection, q.str().c_str());
	PostgreSqlUtils::checkPostgreSqlError(result);

	enum {
		NUM_X, NUM_Y, NUM_Z,
		BLD_X, BLD_Y, BLD_Z,
		COD_X, COD_Y, COD_Z,
		SCALE
	};
	util::point<unsigned int, 3> blockSize(
					boost::lexical_cast<unsigned int>(PQgetvalue(result, 0, BLD_X)),
					boost::lexical_cast<unsigned int>(PQgetvalue(result, 0, BLD_Y)),
					boost::lexical_cast<unsigned int>(PQgetvalue(result, 0, BLD_Z))
			);
	util::point<unsigned int, 3> numBlocks(
		boost::lexical_cast<unsigned int>(PQgetvalue(result, 0, NUM_X)),
		boost::lexical_cast<unsigned int>(PQgetvalue(result, 0, NUM_Y)),
		boost::lexical_cast<unsigned int>(PQgetvalue(result, 0, NUM_Z))
	);
	config.setVolumeSize(numBlocks*blockSize);
	config.setBlockSize(blockSize);
	config.setCoreSize(
			util::point<unsigned int, 3>(
					boost::lexical_cast<unsigned int>(PQgetvalue(result, 0, COD_X)),
					boost::lexical_cast<unsigned int>(PQgetvalue(result, 0, COD_Y)),
					boost::lexical_cast<unsigned int>(PQgetvalue(result, 0, COD_Z))
			));

	unsigned int scale = boost::lexical_cast<unsigned int>(PQgetvalue(result, 0, SCALE));
	rawStackDescription.scale = scale;
	memStackDescription.scale = scale;

	PQclear(result);

	config.setCatmaidStack(Raw, rawStackDescription);
	config.setCatmaidStack(Membrane, memStackDescription);
}

void
PostgreSqlProjectConfigurationStore::fill(StackDescription& stackDescription, StackType stackType) {

	// get stack id

	std::string type = (stackType == Raw ? "Raw" : "Membrane");

	std::stringstream q;
	q <<
			"SELECT project_stack_id "
			"FROM segmentation_stack "
			"WHERE configuration_id=" << stackDescription.segmentationId << " "
			"AND type='" << type << "'";
	PGresult* result = PQexec(_pgConnection, q.str().c_str());
	PostgreSqlUtils::checkPostgreSqlError(result);

	char* stackIdStr = PQgetvalue(result, 0, 0);
	stackDescription.id = boost::lexical_cast<int>(stackIdStr);
	PQclear(result);

	// get complete stack description

	q.str("");
	q <<
			"SELECT dimension, resolution, image_base, file_extension, tile_width, tile_height, tile_source_type "
			"FROM stack WHERE id=" << stackDescription.id;
	result = PQexec(_pgConnection, q.str().c_str());
	PostgreSqlUtils::checkPostgreSqlError(result);

	enum { DIM, RES, IMAGE_BASE, FILE_EXTENSION, TILE_WIDTH, TILE_HEIGHT, TILE_SOURCE_TYPE};

	std::stringstream dimss(PQgetvalue(result, 0, DIM));
	char _;
	dimss >> _;
	dimss >> stackDescription.width;
	dimss >> _;
	dimss >> stackDescription.height;
	dimss >> _;
	dimss >> stackDescription.depth;

	std::stringstream resss(PQgetvalue(result, 0, RES));
	resss >> _;
	resss >> stackDescription.resX;
	resss >> _;
	resss >> stackDescription.resY;
	resss >> _;
	resss >> stackDescription.resZ;

	stackDescription.imageBase      = PQgetvalue(result, 0, IMAGE_BASE);
	stackDescription.fileExtension  = PQgetvalue(result, 0, FILE_EXTENSION);
	stackDescription.tileWidth      = boost::lexical_cast<int>(PQgetvalue(result, 0, TILE_WIDTH));
	stackDescription.tileHeight     = boost::lexical_cast<int>(PQgetvalue(result, 0, TILE_HEIGHT));
	stackDescription.tileSourceType = boost::lexical_cast<int>(PQgetvalue(result, 0, TILE_SOURCE_TYPE));

	PQclear(result);
}
