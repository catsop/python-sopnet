#include "config.h"
#ifdef HAVE_PostgreSQL

#include <boost/tokenizer.hpp>
#include <boost/functional/hash.hpp>
#include <boost/timer/timer.hpp>
#include <util/Logger.h>
#include "PostgreSqlSegmentStore.h"
#include "PostgreSqlUtils.h"

logger::LogChannel postgresqlsegmentstorelog("postgresqlsegmentstorelog", "[PostgreSqlSegmentStore] ");

PostgreSqlSegmentStore::PostgreSqlSegmentStore(
		const ProjectConfiguration& config) : _config(config)
{
	_pgConnection = PostgreSqlUtils::getConnection(
			_config.getPostgreSqlHost(),
			_config.getPostgreSqlPort(),
			_config.getPostgreSqlDatabase(),
			_config.getPostgreSqlUser(),
			_config.getPostgreSqlPassword());
	std::ostringstream q;
	q << "SET search_path TO segstack_"
	  << _config.getCatmaidStack(Membrane).segmentationId
	  << ",public;";
	PQsendQuery(_pgConnection, q.str().c_str());
}

PostgreSqlSegmentStore::~PostgreSqlSegmentStore() {

	if (_pgConnection != 0) {
		PostgreSqlUtils::waitForAsyncQuery(_pgConnection);
		PQfinish(_pgConnection);
	}
}

void
PostgreSqlSegmentStore::associateSegmentsToBlock(
			const SegmentDescriptions& segments,
			const Block&               block) {

	const std::string blockQuery = PostgreSqlUtils::createBlockIdQuery(block);

	if (segments.size() == 0) {
		std::string query =
			"UPDATE block SET segments_flag = TRUE WHERE id = (" + blockQuery + ");";

		PostgreSqlUtils::waitForAsyncQuery(_pgConnection);
		int asyncStatus = PQsendQuery(_pgConnection, query.c_str());
		if (0 == asyncStatus) {
			LOG_ERROR(postgresqlsegmentstorelog) << "PQsendQuery returned 0" << std::endl;
			LOG_ERROR(postgresqlsegmentstorelog) << "The used query was: " << query <<
				std::endl;
		}

		return;
	}

	boost::timer::cpu_timer queryTimer;

	std::ostringstream tmpTableStream;
	tmpTableStream << "_tmp_" << block.x() << "_" << block.y() << "_" << block.z();
	const std::string tmpTable = tmpTableStream.str();

	// Remove any existing segment associations for this block.
	std::ostringstream clearBlockQuery;
	clearBlockQuery <<
			"DELETE FROM segment_block_relation WHERE block_id = (" << blockQuery << ");";

	std::ostringstream segmentQuery;
	segmentQuery << "BEGIN;"
			"CREATE TEMP TABLE segment" << tmpTable <<
			"(LIKE segment) ON COMMIT DROP;"
			"INSERT INTO segment" << tmpTable << " (id, section_sup, "
			"min_x, min_y, max_x, max_y, type) VALUES";

	std::ostringstream sliceQuery;
	sliceQuery <<
			"INSERT INTO segment_slice (segment_id, slice_id, direction) SELECT * FROM (VALUES";

	std::ostringstream segmentFeatureQuery;
	segmentFeatureQuery <<
			"INSERT INTO segment_features (segment_id, features) SELECT * FROM (VALUES";

	std::ostringstream blockSegmentQuery;
	blockSegmentQuery <<
			"INSERT INTO segment_block_relation (block_id, segment_id) "
			"SELECT b.id, t.id FROM (" << blockQuery << ") AS b, "
			"segment" << tmpTable << " AS t "
			"WHERE NOT EXISTS "
			"(SELECT 1 FROM segment_block_relation s WHERE "
			"(s.block_id, s.segment_id) = (b.id, t.id));";

	char separator = ' ';
	char sliceSeparator = ' ';

	foreach (const SegmentDescription& segment, segments) {
		std::string segmentId = boost::lexical_cast<std::string>(
			PostgreSqlUtils::hashToPostgreSqlId(segment.getHash()));
		const util::rect<unsigned int>& segmentBounds = segment.get2DBoundingBox();

		// Create segment.
		segmentQuery << separator << '(' <<
				boost::lexical_cast<std::string>(segmentId) << ',' <<
				boost::lexical_cast<std::string>(segment.getSection()) << ',' <<
				boost::lexical_cast<std::string>(segmentBounds.minX) << ',' <<
				boost::lexical_cast<std::string>(segmentBounds.minY) << ',' <<
				boost::lexical_cast<std::string>(segmentBounds.maxX) << ',' <<
				boost::lexical_cast<std::string>(segmentBounds.maxY) << ',' <<
				boost::lexical_cast<std::string>(segment.getType()) << ')';

		// Associate slices to segment.
		foreach (const SliceHash& hash, segment.getLeftSlices()) {
			sliceQuery << sliceSeparator << '(' <<
				segmentId << ',' <<
				boost::lexical_cast<std::string>(PostgreSqlUtils::hashToPostgreSqlId(hash)) <<
				",TRUE)";
			sliceSeparator = ',';
		}

		foreach (const SliceHash& hash, segment.getRightSlices()) {
			sliceQuery << sliceSeparator << '(' <<
				segmentId << ',' <<
				boost::lexical_cast<std::string>(PostgreSqlUtils::hashToPostgreSqlId(hash)) <<
				",FALSE)";
			sliceSeparator = ',';
		}

		// Store segment features.
		segmentFeatureQuery << separator << '(' << segmentId << ",'";
		char featureSeparator = '{';
		foreach (const double featVal, segment.getFeatures()) {
			segmentFeatureQuery << featureSeparator << boost::lexical_cast<std::string>(featVal);
			featureSeparator = ',';
		}
		segmentFeatureQuery << "}'";
		// Coerce the feature type for the first VALUES entry.
		if (' ' == separator) segmentFeatureQuery << "::double precision[]";
		segmentFeatureQuery << ')';

		separator = ',';
	}

	segmentQuery << ";LOCK TABLE segment IN EXCLUSIVE MODE;";
	segmentQuery <<
			"INSERT INTO segment "
			"(id, section_sup, "
			"min_x, min_y, max_x, max_y, type) "
			"SELECT "
			"t.id, t.section_sup, "
			"t.min_x, t.min_y, t.max_x, t.max_y, t.type "
			"FROM segment" << tmpTable << " AS t "
			"LEFT OUTER JOIN segment s "
			"ON (s.id = t.id) WHERE s.id IS NULL;";
	sliceQuery << " ) AS t (segment_id, slice_id, direction) WHERE NOT EXISTS "
			"(SELECT 1 FROM segment_slice ss "
			"WHERE (ss.segment_id, ss.slice_id) = (t.segment_id, t.slice_id));";
	segmentFeatureQuery << " ) AS t (segment_id, features) WHERE NOT EXISTS "
			"(SELECT 1 FROM segment_features sf "
			"WHERE sf.segment_id = t.segment_id);";

	segmentQuery << sliceQuery.str() << segmentFeatureQuery.str()
			<< clearBlockQuery.str() << blockSegmentQuery.str();

	// Set block flag to show segments have been stored.
	segmentQuery <<
			"UPDATE block SET segments_flag = TRUE WHERE id = (" << blockQuery << ");";
	segmentQuery << "COMMIT;";
	std::string query = segmentQuery.str();
	PostgreSqlUtils::waitForAsyncQuery(_pgConnection);
	int asyncStatus = PQsendQuery(_pgConnection, query.c_str());
	if (0 == asyncStatus) {
		LOG_ERROR(postgresqlsegmentstorelog) << "PQsendQuery returned 0" << std::endl;
		LOG_ERROR(postgresqlsegmentstorelog) << "The used query was: " << query <<
			std::endl;
		UTIL_THROW_EXCEPTION(PostgreSqlException, "PQsendQuery returned 0");
	}

	boost::chrono::nanoseconds queryElapsed(queryTimer.elapsed().wall);
	LOG_DEBUG(postgresqlsegmentstorelog) << "Stored " << segments.size() << " segments in "
			<< (queryElapsed.count() / 1e6) << " ms (wall) ("
			<< (1e9 * segments.size()/queryElapsed.count()) << " segments/s)" << std::endl;
}

boost::shared_ptr<SegmentDescriptions>
PostgreSqlSegmentStore::getSegmentsByBlocks(
			const Blocks& blocks,
			Blocks&       missingBlocks,
			bool          readCosts) {

	boost::shared_ptr<SegmentDescriptions> segmentDescriptions =
			boost::make_shared<SegmentDescriptions>();

	if (blocks.empty()) return segmentDescriptions;

	boost::timer::cpu_timer queryTimer;

	// Check if any requested block do not have slices flagged.
	PostgreSqlUtils::waitForAsyncQuery(_pgConnection);
	std::string blockIdsStr = PostgreSqlUtils::checkBlocksFlags(
			blocks, "segments_flag", missingBlocks, _pgConnection);

	if (!missingBlocks.empty()) return segmentDescriptions;

	// Query the number of features configured for this stack, to verify parsed
	// results and create empty feature sets for user-created segments.
	std::string numFeaturesQuery =
			"SELECT size FROM segmentation_feature_info WHERE segmentation_stack_id = " +
			boost::lexical_cast<std::string>(_config.getCatmaidStack(Membrane).segmentationId);
	PGresult* queryResult = PQexec(_pgConnection, numFeaturesQuery.c_str());

	PostgreSqlUtils::checkPostgreSqlError(queryResult, numFeaturesQuery);
	if (PQntuples(queryResult) != 1 || PQgetisnull(queryResult, 0, 0)) {
		std::string errorMsg = "Feature size is not configured for this stack.";
		LOG_ERROR(postgresqlsegmentstorelog) << errorMsg << std::endl;
		UTIL_THROW_EXCEPTION(PostgreSqlException, errorMsg);
	}

	char* cellStr = PQgetvalue(queryResult, 0, 0);
	PQclear(queryResult);
	const unsigned int numFeatures = boost::lexical_cast<unsigned int>(cellStr);

	// Query segments for this set of blocks
	std::string featureJoin(readCosts ?
			"LEFT OUTER JOIN segment_features sf "
			"ON s.cost IS NULL AND s.id = sf.segment_id " :
			"LEFT JOIN segment_features sf ON s.id = sf.segment_id ");
	std::string blockSegmentsQuery =
			"SELECT s.id, s.section_sup, s.min_x, s.min_y, s.max_x, s.max_y, "
			"s.cost, sf.segment_id, sf.features, " // sf.segment_id is needed for GROUP
			"array_agg(DISTINCT ROW(ss.slice_id, ss.direction)) "
			"FROM segment_block_relation sbr "
			"JOIN segment s ON sbr.segment_id = s.id "
			"JOIN segment_slice ss ON s.id = ss.segment_id "
			+ featureJoin +
			"WHERE sbr.block_id IN (" + blockIdsStr + ") "
			"GROUP BY s.id, sf.segment_id";

	enum { FIELD_ID, FIELD_SECTION, FIELD_MIN_X, FIELD_MIN_Y,
			FIELD_MAX_X, FIELD_MAX_Y,
			FIELD_COST, FIELD_SFID_UNUSED, FIELD_FEATURES, FIELD_SLICE_ARRAY };
	queryResult = PQexec(_pgConnection, blockSegmentsQuery.c_str());

	PostgreSqlUtils::checkPostgreSqlError(queryResult, blockSegmentsQuery);
	boost::chrono::nanoseconds queryElapsed(queryTimer.elapsed().wall);
	int nSegments = PQntuples(queryResult);

	// Build SegmentDescription for each row
	for (int i = 0; i < nSegments; ++i) {
		cellStr = PQgetvalue(queryResult, i, FIELD_ID); // Segment ID
		SegmentHash segmentHash = PostgreSqlUtils::postgreSqlIdToHash(
				boost::lexical_cast<PostgreSqlHash>(cellStr));
		cellStr = PQgetvalue(queryResult, i, FIELD_SECTION); // Z-section supremum
		unsigned int section = boost::lexical_cast<unsigned int>(cellStr);
		cellStr = PQgetvalue(queryResult, i, FIELD_MIN_X);
		unsigned int minX = boost::lexical_cast<unsigned int>(cellStr);
		cellStr = PQgetvalue(queryResult, i, FIELD_MIN_Y);
		unsigned int minY = boost::lexical_cast<unsigned int>(cellStr);
		cellStr = PQgetvalue(queryResult, i, FIELD_MAX_X);
		unsigned int maxX = boost::lexical_cast<unsigned int>(cellStr);
		cellStr = PQgetvalue(queryResult, i, FIELD_MAX_Y);
		unsigned int maxY = boost::lexical_cast<unsigned int>(cellStr);
		SegmentDescription segmentDescription(
				section,
				util::rect<unsigned int>(minX, minY, maxX, maxY));

		boost::char_separator<char> separator("{}()\", \t");

		if (readCosts && !PQgetisnull(queryResult, i, FIELD_COST)) {
			cellStr = PQgetvalue(queryResult, i, FIELD_COST);
			segmentDescription.setCost(boost::lexical_cast<double>(cellStr));
		} else if (!PQgetisnull(queryResult, i, FIELD_FEATURES)) {
			// Parse features of form: {featVal1, featVal2, ...}
			cellStr = PQgetvalue(queryResult, i, FIELD_FEATURES);
			std::string featuresString(cellStr);
			boost::tokenizer<boost::char_separator<char> > features(featuresString, separator);
			std::vector<double> segmentFeatures;
			segmentFeatures.reserve(numFeatures);
			foreach (const std::string& feature, features) {
				segmentFeatures.push_back(boost::lexical_cast<double>(feature));
			}

			segmentDescription.setFeatures(segmentFeatures);
		} else {
			// Non-sopnet-generated segment, no features and assumed to be constrained.
			std::vector<double> segmentFeatures(numFeatures, 0);
			segmentDescription.setFeatures(segmentFeatures);
		}

		// Parse segment->slice tuples for segment of form: {"(slice_id, direction)",...}
		cellStr = PQgetvalue(queryResult, i, FIELD_SLICE_ARRAY);
		std::string tuplesString(cellStr);

		boost::tokenizer<boost::char_separator<char> > tuples(tuplesString, separator);

		for (boost::tokenizer<boost::char_separator<char> >::iterator tuple = tuples.begin();
				tuple != tuples.end();
				++tuple) {

			SliceHash sliceHash = PostgreSqlUtils::postgreSqlIdToHash(
					boost::lexical_cast<PostgreSqlHash>(*tuple));

			std::string direction = *(++tuple);
			bool isLeft = direction.at(0) == 't';

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

	boost::chrono::nanoseconds totalElapsed(queryTimer.elapsed().wall);
	LOG_DEBUG(postgresqlsegmentstorelog) << "Retrieved " << segmentDescriptions->size() << " segments in "
			<< (totalElapsed.count() / 1e6) << " ms (wall) (query: "
			<< (queryElapsed.count() / 1e6) << "ms; "
			<< (1e9 * segmentDescriptions->size()/queryElapsed.count()) << " segments/s)" << std::endl;

	return segmentDescriptions;
}

boost::shared_ptr<SegmentConstraints>
PostgreSqlSegmentStore::getConstraintsByBlocks(
		const Blocks& blocks) {

	boost::shared_ptr<SegmentConstraints> constraints = boost::make_shared<SegmentConstraints>();

	if (blocks.empty()) return constraints;

	boost::timer::cpu_timer queryTimer;

	const std::string blocksQuery = PostgreSqlUtils::createBlockIdQuery(blocks);

	// Query constraints for this set of blocks
	std::string blockConstraintsQuery =
			"SELECT cst.id, cst.relation, cst.value, "
			"array_agg(DISTINCT ROW(csr.segment_id, csr.coefficient)) "
			"FROM solution_constraint cst "
			"JOIN block_constraint_relation bcr ON bcr.constraint_id = cst.id "
			"JOIN constraint_segment_relation csr ON csr.constraint_id = cst.id "
			"WHERE bcr.block_id IN (" + blocksQuery + ") "
			"GROUP BY cst.id";

	enum { FIELD_ID_UNUSED, FIELD_RELATION, FIELD_VALUE, FIELD_SEGMENT_ARRAY };
	PostgreSqlUtils::waitForAsyncQuery(_pgConnection);
	PGresult* queryResult = PQexec(_pgConnection, blockConstraintsQuery.c_str());

	PostgreSqlUtils::checkPostgreSqlError(queryResult, blockConstraintsQuery);
	int nConstraints = PQntuples(queryResult);

	// Build constraint for each row
	for (int i = 0; i < nConstraints; ++i) {
		SegmentConstraint constraint;

		char* cellStr;
		cellStr = PQgetvalue(queryResult, i, FIELD_RELATION);
		Relation relation = cellStr[0] == 'L' ? LessEqual : (cellStr[0] == 'E' ? Equal : GreaterEqual);
		constraint.setRelation(relation);
		cellStr = PQgetvalue(queryResult, i, FIELD_VALUE);
		constraint.setValue(boost::lexical_cast<double>(cellStr));

		// Parse constraint->segment tuples for segment of form: {"(segment_id)",...}
		cellStr = PQgetvalue(queryResult, i, FIELD_SEGMENT_ARRAY);
		std::string segmentsString(cellStr);

		boost::char_separator<char> separator("{}()\", \t");
		boost::tokenizer<boost::char_separator<char> > tuples(segmentsString, separator);
		for (boost::tokenizer<boost::char_separator<char> >::iterator tuple = tuples.begin();
			tuple != tuples.end();
			++tuple) {
			SegmentHash segment = PostgreSqlUtils::postgreSqlIdToHash(
					boost::lexical_cast<PostgreSqlHash>(*tuple));
			double coeff = boost::lexical_cast<double>(*(++tuple));

			constraint.setCoefficient(segment, coeff);
		}

		constraints->push_back(constraint);
	}

	PQclear(queryResult);

	boost::chrono::nanoseconds queryElapsed(queryTimer.elapsed().wall);
	LOG_DEBUG(postgresqlsegmentstorelog) << "Retrieved " << constraints->size() << " constraints in "
			<< (queryElapsed.count() / 1e6) << " ms (wall) ("
			<< (1e9 * constraints->size()/queryElapsed.count()) << " constraints/s)" << std::endl;

	return constraints;
}

std::vector<double>
PostgreSqlSegmentStore::getFeatureWeights() {

	std::string query =
			"SELECT weights FROM segmentation_feature_info WHERE segmentation_stack_id="
			+ boost::lexical_cast<std::string>(_config.getCatmaidStack(Membrane).segmentationId);
	PostgreSqlUtils::waitForAsyncQuery(_pgConnection);
	PGresult* queryResult = PQexec(_pgConnection, query.c_str());
	PostgreSqlUtils::checkPostgreSqlError(queryResult, query);

	int nRows = PQntuples(queryResult);
	if (!nRows) {
		std::string errorMsg = "No feature weights found for stack.";
		LOG_ERROR(postgresqlsegmentstorelog) << errorMsg << std::endl;
		UTIL_THROW_EXCEPTION(PostgreSqlException, errorMsg);
	}

	char* cellStr = PQgetvalue(queryResult, 0, 0);
	std::string weightsString(cellStr);
	weightsString = weightsString.substr(1, weightsString.length() - 2); // Remove { and }
	boost::char_separator<char> separator("{}()\", \t");
	boost::tokenizer<boost::char_separator<char> > weightsTokens(weightsString, separator);

	std::vector<double> weights;

	foreach (const std::string& weight, weightsTokens) {
		weights.push_back(boost::lexical_cast<double>(weight));
	}

	PQclear(queryResult);

	return weights;
}

void
PostgreSqlSegmentStore::storeSegmentCosts(const std::map<SegmentHash, double>& costs) {

	boost::timer::cpu_timer queryTimer;

	std::ostringstream query;
	query << "BEGIN;LOCK TABLE segment IN EXCLUSIVE MODE;";
	query << "UPDATE segment SET cost=t.cost FROM (VALUES";

	char separator = ' ';
	typedef const std::map<SegmentHash, double> costs_type;
	foreach (const costs_type::value_type pair, costs) {
		query << separator
			  << '(' << boost::lexical_cast<std::string>(PostgreSqlUtils::hashToPostgreSqlId(pair.first))
			  << ',' << pair.second << ')';
		separator = ',';
	}

	query << ") AS t (segment_id, cost) "
			"WHERE t.segment_id = segment.id;COMMIT;";
	std::string queryStr = query.str();
	PostgreSqlUtils::waitForAsyncQuery(_pgConnection);
	int asyncStatus = PQsendQuery(_pgConnection, queryStr.c_str());
	if (0 == asyncStatus) {
		LOG_ERROR(postgresqlsegmentstorelog) << "PQsendQuery returned 0" << std::endl;
		LOG_ERROR(postgresqlsegmentstorelog) << "The used query was: " << queryStr <<
			std::endl;
		UTIL_THROW_EXCEPTION(PostgreSqlException, "PQsendQuery returned 0");
	}

	boost::chrono::nanoseconds queryElapsed(queryTimer.elapsed().wall);
	LOG_DEBUG(postgresqlsegmentstorelog) << "Stored " << costs.size() << " segment costs in "
			<< (queryElapsed.count() / 1e6) << " ms (wall) ("
			<< (1e9 * costs.size()/queryElapsed.count()) << " segments/s)" << std::endl;
}

void
PostgreSqlSegmentStore::storeSolution(
		const std::vector<std::set<SegmentHash> >& assemblies,
		const Core& core) {

	boost::timer::cpu_timer queryTimer;
	unsigned int totalSegments = 0;

	const std::string coreQuery = PostgreSqlUtils::createCoreIdQuery(core);
	const std::string solutionQuery =
			"INSERT INTO solution (core_id) (" + coreQuery + ") RETURNING id, core_id;";
	PostgreSqlUtils::waitForAsyncQuery(_pgConnection);
	PGresult* queryResult = PQexec(_pgConnection, solutionQuery.c_str());

	PostgreSqlUtils::checkPostgreSqlError(queryResult, solutionQuery);
	int solutionId = boost::lexical_cast<int>(PQgetvalue(queryResult, 0, 0));
	int coreId = boost::lexical_cast<int>(PQgetvalue(queryResult, 0, 1));
	PQclear(queryResult);

	std::ostringstream query;

	if (!assemblies.empty()) {
		std::vector<SegmentHash> assemblyHashes;
		assemblyHashes.reserve(assemblies.size());

		// Create assemblies and find which are new.
		std::ostringstream assemblyHashStream;
		char separator = ' ';

		foreach (const std::set<SegmentHash> segmentHashes, assemblies) {
			SegmentHash assemblyHash = 0;

			foreach (const SegmentHash& segmentHash, segmentHashes) {
				boost::hash_combine(assemblyHash, segmentHash);
			}

			assemblyHashes.push_back(assemblyHash);
			assemblyHashStream << separator << '('
					<< boost::lexical_cast<std::string>(PostgreSqlUtils::hashToPostgreSqlId(assemblyHash))
					<< ')';
			separator = ',';
		}
		std::string assemblyHashString(assemblyHashStream.str());

		// Lock all assemblies for core so there are no race conditions with
		// concurrent solutions for this core about which assemblies to insert.
		// Once this set has been established the lock can be released even
		// while assembly segments are being inserted.
		std::ostringstream assemblyQuery;
		assemblyQuery << "BEGIN;"
				"SELECT * FROM assembly WHERE core_id=" << coreId << " FOR UPDATE;"
				"INSERT INTO assembly (core_id, hash) "
					"(SELECT " << coreId << ", v.hash FROM (VALUES " << assemblyHashString << ") "
					"AS v (hash) "
					"WHERE NOT EXISTS (SELECT 1 FROM assembly a "
					"WHERE a.core_id = " << coreId << " AND a.hash = v.hash)) "
				"RETURNING id, hash;";
		enum { FIELD_ASSEMBLY_ID, FIELD_ASSEMBLY_HASH };

		std::string assemblyQueryString(assemblyQuery.str());
		queryResult = PQexec(_pgConnection, assemblyQueryString.c_str());
		PostgreSqlUtils::checkPostgreSqlError(queryResult, assemblyQueryString);

		// Commit the transaction while further queries are prepared.
		int asyncStatus = PQsendQuery(_pgConnection, "COMMIT;");
		if (0 == asyncStatus) {
			LOG_ERROR(postgresqlsegmentstorelog) << "PQsendQuery returned 0" << std::endl;
			LOG_ERROR(postgresqlsegmentstorelog) << "The used query was: COMMIT;" <<
				std::endl;
			UTIL_THROW_EXCEPTION(PostgreSqlException, "PQsendQuery returned 0");
		}

		int nNewAssemblies = PQntuples(queryResult);

		std::map<SegmentHash, int> assemblyIdMap;

		// Parse assemblies.
		for (int i = 0; i < nNewAssemblies; ++i) {
			char* cellStr;
			cellStr = PQgetvalue(queryResult, i, FIELD_ASSEMBLY_ID);
			int assemblyId = boost::lexical_cast<int>(cellStr);
			cellStr = PQgetvalue(queryResult, i, FIELD_ASSEMBLY_HASH);
			SegmentHash assemblyHash = PostgreSqlUtils::postgreSqlIdToHash(
					boost::lexical_cast<PostgreSqlHash>(cellStr));

			assemblyIdMap[assemblyHash] = assemblyId;
		}

		PQclear(queryResult);

		// Insert assembly solution relationships.
		query << "INSERT INTO solution_assembly (solution_id, assembly_id) "
				"SELECT " << solutionId << ",a.id FROM assembly a "
				"JOIN (VALUES " << assemblyHashString << ") AS v (hash) "
				"ON (a.core_id = " << coreId << " AND a.hash = v.hash);";

		if (nNewAssemblies > 0) {

			// Insert assembly segment relationships only for new assemblies.
			query << "INSERT INTO assembly_segment (assembly_id, segment_id) VALUES";

			separator = ' ';
			for (unsigned int i = 0; i < assemblies.size(); ++i) {

				std::map<SegmentHash, int>::iterator it;
				it = assemblyIdMap.find(assemblyHashes[i]);

				if (it != assemblyIdMap.end()) {

					int assemblyId = it->second;

					foreach (const SegmentHash& segmentHash, assemblies[i]) {

						query << separator << '('
							  << boost::lexical_cast<std::string>(assemblyId) << ','
							  << boost::lexical_cast<std::string>(PostgreSqlUtils::hashToPostgreSqlId(segmentHash))
							  << ')';
						separator = ',';
					}

					totalSegments += assemblies[i].size();
				}
			}

			query << ";";
		}
	}

	// Mark this solution as precedent.
	query << "INSERT INTO solution_precedence (core_id, solution_id) "
			"VALUES (" << coreId << ',' << solutionId << ");"
			"UPDATE core SET solution_set_flag = TRUE "
			"WHERE id = " << coreId;

	std::string queryString(query.str());
	PostgreSqlUtils::waitForAsyncQuery(_pgConnection);
	queryResult = PQexec(_pgConnection, queryString.c_str());

	PostgreSqlUtils::checkPostgreSqlError(queryResult, queryString);
	PQclear(queryResult);

	boost::chrono::nanoseconds queryElapsed(queryTimer.elapsed().wall);
	LOG_DEBUG(postgresqlsegmentstorelog) << "Stored " << totalSegments << " segments ("
			<< assemblies.size() << " assemblies) in "
			<< (queryElapsed.count() / 1e6) << " ms (wall) ("
			<< (1e9 * totalSegments/queryElapsed.count()) << " segments/s)" << std::endl;
}

bool
PostgreSqlSegmentStore::getSegmentsFlag(const Block& block) {

	const std::string blockQuery = PostgreSqlUtils::createBlockIdQuery(block);
	std::string blockFlagQuery = "SELECT segments_flag FROM block "
			"WHERE id = (" + blockQuery + ")";
	PostgreSqlUtils::waitForAsyncQuery(_pgConnection);
	PGresult* queryResult = PQexec(_pgConnection, blockFlagQuery.c_str());

	PostgreSqlUtils::checkPostgreSqlError(queryResult, blockFlagQuery);

	bool result = 0 == strcmp(PQgetvalue(queryResult, 0, 0), "t");

	PQclear(queryResult);

	return result;
}

#endif //HAVE_PostgreSQL
