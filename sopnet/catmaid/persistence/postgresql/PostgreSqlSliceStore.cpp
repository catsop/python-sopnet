#include "config.h"
#ifdef HAVE_PostgreSQL

#include <fstream>
#include <catmaid/persistence/django/DjangoUtils.h>
#include <imageprocessing/ConnectedComponent.h>
#include <imageprocessing/Image.h>
#include <sopnet/slices/ComponentTreeConverter.h>
#include <util/ProgramOptions.h>
#include <util/httpclient.h>
#include <util/point.hpp>
#include "PostgreSqlSliceStore.h"

logger::LogChannel postgresqlslicestorelog("postgresqlslicestorelog", "[PostgreSqlSliceStore] ");

PostgreSqlSliceStore::PostgreSqlSliceStore(
		const boost::shared_ptr<DjangoBlockManager> blockManager,
		const std::string& componentDirectory,
		const std::string& pgHost,
		const std::string& pgUser,
		const std::string& pgPassword,
		const std::string& pgDatabaseName) :

	_server(blockManager->getServer()),
	_stack(blockManager->getStack()),
	_project(blockManager->getProject()),
	_blockManager(blockManager),
	_componentDirectory(componentDirectory)
{
	std::string connectionInfo =
			(pgHost.empty()         ? "" : "host="     + pgHost         + " ") +
			(pgDatabaseName.empty() ? "" : "dbname="   + pgDatabaseName + " ") +
			(pgUser.empty()         ? "" : "user="     + pgUser         + " ") +
			(pgPassword.empty()     ? "" : "password=" + pgPassword     + " ");

	_pgConnection = PQconnectdb(connectionInfo.c_str());

	/* Check to see that the backend connection was successfully made */
	if (PQstatus(_pgConnection) != CONNECTION_OK) {

		UTIL_THROW_EXCEPTION(
				PostgreSqlException,
				"Connection to database failed: " << PQerrorMessage(_pgConnection));
	}
}

PostgreSqlSliceStore::~PostgreSqlSliceStore() {

	if (_pgConnection != 0)
		PQfinish(_pgConnection);
}

void
PostgreSqlSliceStore::associate(boost::shared_ptr<Slices> slices, boost::shared_ptr<Block> block) {

	if (slices->size() == 0)
		return;

	// create slices in slice table

	// store associations with block
}

boost::shared_ptr<Slices>
PostgreSqlSliceStore::retrieveSlices(const Blocks& blocks) {

	boost::shared_ptr<Slices> slices = boost::make_shared<Slices>();

	// get all slices that are associated with the given blocks

	return slices;
}


boost::shared_ptr<Blocks>
PostgreSqlSliceStore::getAssociatedBlocks(boost::shared_ptr<Slice> slice) {

	boost::shared_ptr<Blocks> blocks = boost::make_shared<Blocks>();
	
	return blocks;
}

void
PostgreSqlSliceStore::storeConflict(boost::shared_ptr<ConflictSets> conflictSets) {

	// get the hashes of the slices in the conflict sets

	// for now:
	//
	//   get the postgre ids for the slices by their hashes
	//   create a new conflict set, associate its id with the slice ids

	// later:
	//
	//   store the slice hashes, avoid a lookup on the postgre ids
}

boost::shared_ptr<ConflictSets>
PostgreSqlSliceStore::retrieveConflictSets(const Slices& slices)
{
	boost::shared_ptr<ConflictSets> conflictSets = boost::make_shared<ConflictSets>();

	// get all conflict sets
	//
	// Conflict sets are lists of sopnet slice ids. To get them, we assume all 
	// involved slices have already been retrieved. This way we can use the 
	// slice hash (from the database) to map to the real slice (as queried 
	// earlier) for which we can get the id.

	return conflictSets;
}

void
PostgreSqlSliceStore::dumpStore() {
}

std::string
PostgreSqlSliceStore::generateSliceHash(const Slice& slice) {

	return boost::lexical_cast<std::string>(slice.hashValue());
}

void
PostgreSqlSliceStore::putSlice(boost::shared_ptr<Slice> slice, const std::string hash) {

	if (!_idSliceMap.count(slice->getId()))
	{
		_sliceHashMap[*slice] = hash;
		_hashSliceMap[hash] = slice;
		_idSliceMap[slice->getId()] = slice;
	}
}

std::string
PostgreSqlSliceStore::getHash(const Slice& slice) {

	if (_sliceHashMap.count(slice))
	{
		return _sliceHashMap[slice];
	}
	else
	{
		std::string hash = generateSliceHash(slice);
		_sliceHashMap[slice] = hash;
		return hash;
	}
}

void
PostgreSqlSliceStore::saveConnectedComponent(std::string sliceHash, const ConnectedComponent& component) {

	std::ofstream componentFile((_componentDirectory + "/" + sliceHash + ".cmp").c_str());

	// store the component's value
	componentFile << component.getValue() << " ";

	foreach (const util::point<unsigned int>& p, component.getPixels())
		componentFile << p.x << " " << p.y << " ";
}

boost::shared_ptr<ConnectedComponent>
PostgreSqlSliceStore::readConnectedComponent(std::string sliceHash) {

	// create an empty pixel list
	boost::shared_ptr<ConnectedComponent::pixel_list_type> component = 
		boost::make_shared<ConnectedComponent::pixel_list_type>();

	// open the file
	std::ifstream componentFile((_componentDirectory + "/" + sliceHash + ".cmp").c_str());

	// read the component's value
	double value;
	componentFile >> value;

	// read the pixel list
	while (!componentFile.eof() && componentFile.good()) {

		unsigned int x, y;

		if (!(componentFile >> x)) break;
		if (!(componentFile >> y)) break;

		component->push_back(util::point<unsigned int>(x, y));
	}

	// Create the component
	return boost::make_shared<ConnectedComponent>(
			boost::shared_ptr<Image>(), /* no image */
			value,
			component,
			0,
			component->size());
		
}

boost::shared_ptr<Slice>
PostgreSqlSliceStore::sliceByHash(const std::string& hash)
{

	LOG_ALL(postgresqlslicestorelog) << "asking for slice with hash " << hash << std::endl;

	if (_hashSliceMap.count(hash))
	{
		LOG_ALL(postgresqlslicestorelog) << "slice found" << std::endl;
		return _hashSliceMap[hash];
	}
	else
	{
		LOG_ALL(postgresqlslicestorelog) << "this slice does not exist -- return null-pointer" << std::endl;

		boost::shared_ptr<Slice> dummySlice = boost::shared_ptr<Slice>();
		return dummySlice;
	}
}

boost::shared_ptr<DjangoBlockManager>
PostgreSqlSliceStore::getDjangoBlockManager() const
{
	return _blockManager;
}

#endif // HAVE_PostgreSQL
