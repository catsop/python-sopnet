#include "config.h"
#ifdef HAVE_PostgreSQL

#include <boost/tokenizer.hpp>
#include <util/Logger.h>
#include "PostgreSqlSegmentStore.h"
#include "PostgreSqlUtils.h"

logger::LogChannel postgresqlsegmentstorelog("postgresqlsegmentstorelog", "[PostgreSqlSegmentStore] ");

PostgreSqlSegmentStore::PostgreSqlSegmentStore(
		const ProjectConfiguration& config) : _config(config), _blockUtils(_config)
{
	_pgConnection = PostgreSqlUtils::getConnection(
			_config.getPostgreSqlHost(),
			_config.getPostgreSqlDatabase(),
			_config.getPostgreSqlUser(),
			_config.getPostgreSqlPassword());
}

PostgreSqlSegmentStore::~PostgreSqlSegmentStore() {

	if (_pgConnection != 0)
		PQfinish(_pgConnection);
}

void
PostgreSqlSegmentStore::associateSegmentsToBlock(
			const SegmentDescriptions& segments,
			const Block&               block) {

	PGresult* queryResult;
	const std::string blockQuery = PostgreSqlUtils::createBlockIdQuery(
				_blockUtils, block, _config.getCatmaidRawStackId());

	// Remove any existing segment associations for this block.
	std::string clearBlockQuery =
			"DELETE FROM djsopnet_segmentblockrelation WHERE id = (" + blockQuery + ")";
	queryResult = PQexec(_pgConnection, clearBlockQuery.c_str());

	PostgreSqlUtils::checkPostgreSqlError(queryResult, clearBlockQuery);
	PQclear(queryResult);

	foreach (const SegmentDescription& segment, segments) {
		// TODO: this transaction is too granular for performance but useful for debug
		queryResult = PQexec(_pgConnection, "BEGIN"); // Begin transaction
		PQclear(queryResult);

		std::string segmentId = boost::lexical_cast<std::string>(
			PostgreSqlUtils::hashToPostgreSqlId(segment.getHash()));
		const util::rect<unsigned int>& segmentBounds = segment.get2DBoundingBox();
		const util::point<double>& segmentCenter = segment.getCenter();

		// Create segment.
		std::string segmentQuery =
				"INSERT INTO djsopnet_segment (id, stack_id, section_inf, "
				"min_x, min_y, max_x, max_y, ctr_x, ctr_y, type) VALUES (" +
				boost::lexical_cast<std::string>(segmentId) + ", " +
				boost::lexical_cast<std::string>(_config.getCatmaidRawStackId()) + ", " +
				boost::lexical_cast<std::string>(segment.getSection()) + ", " +
				boost::lexical_cast<std::string>(segmentBounds.minX) + ", " +
				boost::lexical_cast<std::string>(segmentBounds.minY) + ", " +
				boost::lexical_cast<std::string>(segmentBounds.maxX) + ", " +
				boost::lexical_cast<std::string>(segmentBounds.maxY) + ", " +
				boost::lexical_cast<std::string>(segmentCenter.x) + ", " +
				boost::lexical_cast<std::string>(segmentCenter.y) + ", " +
				boost::lexical_cast<std::string>(segment.getType()) + ");";
		queryResult = PQexec(_pgConnection, segmentQuery.c_str());

		PostgreSqlUtils::checkPostgreSqlError(queryResult, segmentQuery);
		PQclear(queryResult);

		// Associate slices to segment.
		foreach (const SliceHash& hash, segment.getLeftSlices()) {
			std::string sliceQuery = "INSERT INTO djsopnet_segmentslice "
				"(segment_id, slice_id, direction) VALUES (" +
				segmentId + "," +
				boost::lexical_cast<std::string>(PostgreSqlUtils::hashToPostgreSqlId(hash)) + ",TRUE);";
			queryResult = PQexec(_pgConnection, sliceQuery.c_str());

			PostgreSqlUtils::checkPostgreSqlError(queryResult, sliceQuery);
			PQclear(queryResult);
		}

		foreach (const SliceHash& hash, segment.getRightSlices()) {
			std::string sliceQuery = "INSERT INTO djsopnet_segmentslice "
				"(segment_id, slice_id, direction) VALUES (" +
				segmentId + "," +
				boost::lexical_cast<std::string>(PostgreSqlUtils::hashToPostgreSqlId(hash)) + ",FALSE);";
			queryResult = PQexec(_pgConnection, sliceQuery.c_str());

			PostgreSqlUtils::checkPostgreSqlError(queryResult, sliceQuery);
			PQclear(queryResult);
		}

		// Store segment features.
		std::ostringstream segmentFeatureQuery;
		segmentFeatureQuery << "INSERT INTO djsopnet_segmentfeatures (segment_id, features) "
				"VALUES (" << segmentId << ", '{";
		char separator = ' ';
		foreach (const double featVal, segment.getFeatures()) {
			segmentFeatureQuery << separator << boost::lexical_cast<std::string>(featVal);
			separator = ',';
		}
		segmentFeatureQuery << "}')";

		queryResult = PQexec(_pgConnection, segmentFeatureQuery.str().c_str());

		PostgreSqlUtils::checkPostgreSqlError(queryResult, segmentFeatureQuery.str());
		PQclear(queryResult);

		// Associate segment to block.
		std::string blockSegmentQuery =
				"INSERT INTO djsopnet_segmentblockrelation (block_id, segment_id) VALUES ((" +
				blockQuery + "), " + segmentId + ");";
		queryResult = PQexec(_pgConnection, blockSegmentQuery.c_str());

		PostgreSqlUtils::checkPostgreSqlError(queryResult, blockSegmentQuery);
		PQclear(queryResult);

		queryResult = PQexec(_pgConnection, "END"); // End transaction
		PQclear(queryResult);
	}

	// Set block flag to show segments have been stored.
	std::string blockFlagQuery =
			"UPDATE djsopnet_block SET segments_flag = TRUE WHERE id = (" + blockQuery + ")";
	queryResult = PQexec(_pgConnection, blockFlagQuery.c_str());

	PostgreSqlUtils::checkPostgreSqlError(queryResult, blockFlagQuery);
	PQclear(queryResult);
}

boost::shared_ptr<SegmentDescriptions>
PostgreSqlSegmentStore::getSegmentsByBlocks(
			const Blocks& blocks,
			Blocks&       missingBlocks) {

	boost::shared_ptr<SegmentDescriptions> segmentDescriptions =
			boost::make_shared<SegmentDescriptions>();

	if (blocks.empty()) return segmentDescriptions;

	std::ostringstream blockIds;
	PGresult* queryResult;

	// Check if any requested block do not have segments stored and flagged.
	foreach (const Block& block, blocks) {
		const std::string blockQuery = PostgreSqlUtils::createBlockIdQuery(
				_blockUtils, block, _config.getCatmaidRawStackId());
		std::string blockFlagQuery = "SELECT id, segments_flag FROM djsopnet_block "
				"WHERE id = (" + blockQuery + ")";
		queryResult = PQexec(_pgConnection, blockFlagQuery.c_str());

		PostgreSqlUtils::checkPostgreSqlError(queryResult, blockFlagQuery);

		if (0 != strcmp(PQgetvalue(queryResult, 0, 1), "t")) {
			missingBlocks.add(block);
		}

		blockIds << PQgetvalue(queryResult, 0, 0) << ",";

		PQclear(queryResult);
	}

	if (!missingBlocks.empty()) return segmentDescriptions;

	std::string blockIdsStr = blockIds.str();
	blockIdsStr.erase(blockIdsStr.length() - 1); // Remove trailing comma.

	// Query segments for this set of blocks
	std::string blockSegmentsQuery =
			"SELECT s.id, s.section_inf, s.min_x, s.min_y, s.max_x, s.max_y, "
			"s.ctr_x, s.ctr_y, sf.id, sf.features, " // sf.id is needed for GROUP
			"array_agg(ROW(ss.slice_id, ss.direction)) "
			"FROM djsopnet_segmentblockrelation sbr "
			"JOIN djsopnet_segment s ON sbr.segment_id = s.id "
			"JOIN djsopnet_segmentslice ss ON s.id = ss.segment_id "
			"JOIN djsopnet_segmentfeatures sf ON s.id = sf.segment_id "
			"WHERE sbr.block_id IN (" + blockIdsStr + ") "
			"GROUP BY sbr.block_id, s.id, sf.id";

	enum { FIELD_ID, FIELD_SECTION, FIELD_MIN_X, FIELD_MIN_Y,
			FIELD_MAX_X, FIELD_MAX_Y, FIELD_CTR_X, FIELD_CTR_Y,
			FIELD_SFID_UNUSED, FIELD_FEATURES, FIELD_SLICE_ARRAY };
	queryResult = PQexec(_pgConnection, blockSegmentsQuery.c_str());

	PostgreSqlUtils::checkPostgreSqlError(queryResult, blockSegmentsQuery);
	int nSegments = PQntuples(queryResult);

	// Build SegmentDescription for each row
	for (int i = 0; i < nSegments; ++i) {
		char* cellStr;
		cellStr = PQgetvalue(queryResult, i, FIELD_ID); // Segment ID
		SegmentHash segmentHash = PostgreSqlUtils::postgreSqlIdToHash(
				boost::lexical_cast<PostgreSqlHash>(cellStr));
		cellStr = PQgetvalue(queryResult, i, FIELD_SECTION); // Z-section infimum
		unsigned int section = boost::lexical_cast<unsigned int>(cellStr);
		cellStr = PQgetvalue(queryResult, i, FIELD_MIN_X);
		unsigned int minX = boost::lexical_cast<unsigned int>(cellStr);
		cellStr = PQgetvalue(queryResult, i, FIELD_MIN_Y);
		unsigned int minY = boost::lexical_cast<unsigned int>(cellStr);
		cellStr = PQgetvalue(queryResult, i, FIELD_MAX_X);
		unsigned int maxX = boost::lexical_cast<unsigned int>(cellStr);
		cellStr = PQgetvalue(queryResult, i, FIELD_MAX_Y);
		unsigned int maxY = boost::lexical_cast<unsigned int>(cellStr);
		cellStr = PQgetvalue(queryResult, i, FIELD_CTR_X);
		double ctrX = boost::lexical_cast<double>(cellStr);
		cellStr = PQgetvalue(queryResult, i, FIELD_CTR_Y);
		double ctrY = boost::lexical_cast<double>(cellStr);
		SegmentDescription segmentDescription(
				section,
				util::rect<unsigned int>(minX, minY, maxX, maxY),
				util::point<double>(ctrX, ctrY));

		// Parse features of form: {featVal1, featVal2, ...}
		cellStr = PQgetvalue(queryResult, i, FIELD_FEATURES);
		std::string featuresString(cellStr);
		featuresString = featuresString.substr(1, featuresString.length() - 2); // Remove { and }
		boost::tokenizer<boost::char_delimiters_separator<char> > features(featuresString);
		std::vector<double> segmentFeatures;
		foreach (const std::string& feature, features) {
			segmentFeatures.push_back(boost::lexical_cast<double>(feature));
		}

		segmentDescription.setFeatures(segmentFeatures);

		// Parse segment->slice tuples for segment of form: {"(slice_id, direction)",...}
		cellStr = PQgetvalue(queryResult, i, FIELD_SLICE_ARRAY);
		std::string tuplesString(cellStr);

		tuplesString = tuplesString.substr(1, tuplesString.length() - 2); // Remove { and }
		boost::tokenizer<boost::char_delimiters_separator<char> > tuples(tuplesString);

		for (boost::tokenizer<boost::char_delimiters_separator<char> >::iterator tuple = tuples.begin();
				tuple != tuples.end();
				++tuple) {

			std::string sliceId = *tuple;
			sliceId = sliceId.substr(sliceId.find_first_of("0123456789-"));

			SliceHash sliceHash = PostgreSqlUtils::postgreSqlIdToHash(
					boost::lexical_cast<PostgreSqlHash>(sliceId));

			std::string direction = *(++tuple);
			bool isLeft = direction.at(direction.find_first_of("tf")) == 't';

			if (isLeft) segmentDescription.addLeftSlice(sliceHash);
			else segmentDescription.addRightSlice(sliceHash);
		}

		// Check that the loaded segment has the correct hash.
		if (segmentDescription.getHash() != segmentHash) {
			std::ostringstream errorMsg;
			errorMsg << "Retrieved segment has wrong hash. Original: " << segmentHash <<
					" Retrieved: " << segmentDescription.getHash() << std::endl;
			errorMsg << "Retrieved segment left slice hashes: ";
			foreach (SliceHash hash, segmentDescription.getLeftSlices()) errorMsg << hash << " ";
			errorMsg << std::endl << "Retrieved segment right slice hashes: ";
			foreach (SliceHash hash, segmentDescription.getRightSlices()) errorMsg << hash << " ";

			LOG_ERROR(postgresqlsegmentstorelog) << errorMsg.str() << std::endl;
			UTIL_THROW_EXCEPTION(PostgreSqlException, errorMsg.str());
		}

		segmentDescriptions->add(segmentDescription);
	}

	PQclear(queryResult);

	return segmentDescriptions;
}

void
PostgreSqlSegmentStore::storeSolution(
		const std::vector<SegmentHash>& segmentHashes,
		const Core& core) {

	const std::string coreQuery = PostgreSqlUtils::createCoreIdQuery(
				_blockUtils, core, _config.getCatmaidRawStackId());

	std::string clearQuery =
			"DELETE FROM djsopnet_segmentsolution WHERE core_id = (" + coreQuery + ")";
	PGresult* queryResult = PQexec(_pgConnection, clearQuery.c_str());

	PostgreSqlUtils::checkPostgreSqlError(queryResult, clearQuery);
	PQclear(queryResult);

	foreach (const SegmentHash& segmentHash, segmentHashes) {
		std::string markSolutionQuery=
				"INSERT INTO djsopnet_segmentsolution"
				"(core_id, segment_id) VALUES ((" + coreQuery + "), " +
				boost::lexical_cast<std::string>(PostgreSqlUtils::hashToPostgreSqlId(segmentHash)) + ")";

		queryResult = PQexec(_pgConnection, markSolutionQuery.c_str());

		PostgreSqlUtils::checkPostgreSqlError(queryResult, markSolutionQuery);
		PQclear(queryResult);
	}
}

bool
PostgreSqlSegmentStore::getSegmentsFlag(const Block& block) {

	const std::string blockQuery = PostgreSqlUtils::createBlockIdQuery(
				_blockUtils, block, _config.getCatmaidRawStackId());
	std::string blockFlagQuery = "SELECT segments_flag FROM djsopnet_block "
			"WHERE id = (" + blockQuery + ")";
	PGresult* queryResult = PQexec(_pgConnection, blockFlagQuery.c_str());

	PostgreSqlUtils::checkPostgreSqlError(queryResult, blockFlagQuery);

	bool result = 0 == strcmp(PQgetvalue(queryResult, 0, 0), "t");

	PQclear(queryResult);

	return result;
}

#endif //HAVE_PostgreSQL
