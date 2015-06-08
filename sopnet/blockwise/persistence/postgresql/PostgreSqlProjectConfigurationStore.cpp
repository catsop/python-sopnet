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

	fillStackDescriptions(config);

	// fill project configuration

	std::stringstream q;
	q <<
			"SELECT bi.num_x, bi.num_y, bi.num_z, "
			"bi.block_dim_x, bi.block_dim_y, bi.block_dim_z, "
			"bi.core_dim_x, bi.core_dim_y, bi.core_dim_z, bi.scale "
			"FROM segmentation_block_info bi "
			"WHERE bi.configuration_id=" << config.getSegmentationConfigurationId();
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
	config.setBlockSize(blockSize);
	config.setCoreSize(
			util::point<unsigned int, 3>(
					boost::lexical_cast<unsigned int>(PQgetvalue(result, 0, COD_X)),
					boost::lexical_cast<unsigned int>(PQgetvalue(result, 0, COD_Y)),
					boost::lexical_cast<unsigned int>(PQgetvalue(result, 0, COD_Z))
			));

	unsigned int scale = boost::lexical_cast<unsigned int>(PQgetvalue(result, 0, SCALE));

	StackDescription rawStackDescription = config.getCatmaidStack(Raw);
	StackDescription memStackDescription = config.getCatmaidStack(Membrane);

	util::point<unsigned int, 3> volumeSize = numBlocks*blockSize;
	volumeSize.z() = std::min(volumeSize.z(), rawStackDescription.depth);
	config.setVolumeSize(volumeSize);

	rawStackDescription.scale = scale;
	memStackDescription.scale = scale;

	PQclear(result);

	config.setCatmaidStack(Raw, rawStackDescription);
	config.setCatmaidStack(Membrane, memStackDescription);
}

void
PostgreSqlProjectConfigurationStore::fillStackDescriptions(ProjectConfiguration& config) {

	// get complete stack description

	std::stringstream q;
	q <<
			"SELECT ss.id, ss.type, s.id, s.dimension, s.resolution, "
			"s.image_base, s.file_extension, "
			"s.tile_width, s.tile_height, s.tile_source_type "
			"FROM segmentation_stack ss "
			"JOIN project_stack ps ON ps.id = ss.project_stack_id "
			"JOIN stack s ON s.id = ps.stack_id "
			"WHERE ss.configuration_id=" << config.getSegmentationConfigurationId();
	PGresult* result = PQexec(_pgConnection, q.str().c_str());
	PostgreSqlUtils::checkPostgreSqlError(result);

	enum {
		SEGMENTATION_STACK_ID, TYPE, STACK_ID,
		DIM, RES, IMAGE_BASE, FILE_EXTENSION,
		TILE_WIDTH, TILE_HEIGHT, TILE_SOURCE_TYPE
	};

	int nStacks = PQntuples(result);

	for (int i = 0; i < nStacks; ++i) {

		StackDescription stackDescription;
		StackType type;

		std::string typeString = boost::lexical_cast<std::string>(PQgetvalue(result, i, TYPE));
		if ("Raw" == typeString) type = Raw;
		else if ("Membrane" == typeString) type = Membrane;
		else {
			UTIL_THROW_EXCEPTION(PostgreSqlException, "Unknown segmentation stack type: " + typeString);
		}

		std::stringstream dimss(PQgetvalue(result, i, DIM));
		char _;
		dimss >> _;
		dimss >> stackDescription.width;
		dimss >> _;
		dimss >> stackDescription.height;
		dimss >> _;
		dimss >> stackDescription.depth;

		std::stringstream resss(PQgetvalue(result, i, RES));
		resss >> _;
		resss >> stackDescription.resX;
		resss >> _;
		resss >> stackDescription.resY;
		resss >> _;
		resss >> stackDescription.resZ;

		stackDescription.segmentationId = boost::lexical_cast<int>(PQgetvalue(result, i, SEGMENTATION_STACK_ID));
		stackDescription.id             = boost::lexical_cast<int>(PQgetvalue(result, i, STACK_ID));
		stackDescription.imageBase      = PQgetvalue(result, i, IMAGE_BASE);
		stackDescription.fileExtension  = PQgetvalue(result, i, FILE_EXTENSION);
		stackDescription.tileWidth      = boost::lexical_cast<int>(PQgetvalue(result, i, TILE_WIDTH));
		stackDescription.tileHeight     = boost::lexical_cast<int>(PQgetvalue(result, i, TILE_HEIGHT));
		stackDescription.tileSourceType = boost::lexical_cast<int>(PQgetvalue(result, i, TILE_SOURCE_TYPE));

		config.setCatmaidStack(type, stackDescription);
	}

	PQclear(result);
}
