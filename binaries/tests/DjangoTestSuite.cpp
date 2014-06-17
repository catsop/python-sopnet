#include "DjangoTestSuite.h"
#include <binaries/tests/DjangoGenerators.h>
#include <catmaid/django/DjangoUtils.h>
#include <catmaid/persistence/StackStore.h>
#include <catmaid/persistence/LocalStackStore.h>
#include <catmaid/django/DjangoBlockManager.h>
#include <catmaid/django/DjangoSliceStore.h>

#include <util/Logger.h>

logger::LogChannel djangotestlog("djangotestlog", "[DjangoTestSuite] ");

namespace catsoptest
{
	
util::ProgramOption optionDjangoTestMembranesPath(
util::_module = 			"core",
util::_long_name = 			"djangoTestMembranes",
util::_description_text = 	"Path to membrane image stack",
util::_default_value =		"./membranes");

DjangoBlockManagerFactory::DjangoBlockManagerFactory(const std::string server,
													 const unsigned int project,
													 const unsigned int stack):
													 _server(server),
													 _project(project),
													 _stack(stack)
{

}


boost::shared_ptr<BlockManager>
DjangoBlockManagerFactory::createBlockManager(const util::point3<unsigned int> blockSize,
											  const util::point3<unsigned int> coreSizeInBlocks)
{
	return catsoptest::getNewDjangoBlockManager(_server, _project, _stack,
												blockSize, coreSizeInBlocks);
}


DjangoSliceStoreFactory::DjangoSliceStoreFactory(const std::string& url,
												 unsigned int project,
												 unsigned int stack) :
												 _url(url), _project(project), _stack(stack)
{
	
}


boost::shared_ptr<SliceStore>
DjangoSliceStoreFactory::createSliceStore()
{
	boost::shared_ptr<DjangoBlockManager> manager = 
		DjangoBlockManager::getBlockManager(_url, _stack, _project);
	boost::shared_ptr<SliceStore> store = boost::make_shared<DjangoSliceStore>(manager);
	return store;
}

boost::shared_ptr<TestSuite>
DjangoTestSuite::djangoTestSuite(const std::string& url, unsigned int project, unsigned int stack)
{
	LOG_USER(djangotestlog) << "Running django test suite" << std::endl;
	boost::shared_ptr<TestSuite> suite = boost::make_shared<TestSuite>("Django");
	
	addBlockManagerTest(suite, url, project, stack);
	addSliceStoreTest(suite, url, project, stack);
	
	return suite;
}

void
DjangoTestSuite::addBlockManagerTest(const boost::shared_ptr<TestSuite> suite,
									 const std::string& url, unsigned int project,
									 unsigned int stack)
{
	util::point3<unsigned int> stackSize = *DjangoUtils::getStackSize(url, project, stack);
	boost::shared_ptr<BlockManagerFactory> factory =
		boost::make_shared<DjangoBlockManagerFactory>(url, project, stack);
	
	boost::shared_ptr<Test<BlockManagerTestParam> > test =
		boost::make_shared<BlockManagerTest>(factory);
	
	LOG_USER(djangotestlog) << "For server " << url << ", project " << project << ", and stack "
		<< stack << ", got stack size: " << stackSize << std::endl;
		
	if (stackSize > util::point3<unsigned int>(0,0,0))
	{
		LOG_USER(djangotestlog) << "Adding Django block manager test" << std::endl;
		suite->addTest<BlockManagerTestParam>(
			test, BlockManagerTest::generateTestParameters(stackSize));
	}
}

void
DjangoTestSuite::addSliceStoreTest(const boost::shared_ptr<TestSuite> suite,
								   const std::string& url, unsigned int project,
								   unsigned int stack)
{
	std::string membranePath = optionDjangoTestMembranesPath.as<std::string>();
	boost::shared_ptr<StackStore> stackStore = boost::make_shared<LocalStackStore>(membranePath);
	util::point3<unsigned int> stackSize = *DjangoUtils::getStackSize(url, project, stack);
	boost::shared_ptr<BlockManagerFactory> blockManagerFactory =
		boost::make_shared<DjangoBlockManagerFactory>(url, project, stack);
	boost::shared_ptr<SliceStoreFactory> sliceStoreFactory =
		boost::make_shared<DjangoSliceStoreFactory>(url, project, stack);
	boost::shared_ptr<Test<SliceStoreTestParam> > test =
		boost::make_shared<SliceStoreTest>(sliceStoreFactory);
		
	LOG_USER(djangotestlog) << "For server " << url << ", project " << project << ", and stack "
		<< stack << ", got stack size: " << stackSize << std::endl;
		
	if (stackSize > util::point3<unsigned int>(0,0,0))
	{
		LOG_USER(djangotestlog) << "Adding Django slice store test" << std::endl;
		suite->addTest<SliceStoreTestParam>(test, SliceStoreTest::generateTestParameters(
			"django", stackSize, stackStore, blockManagerFactory));
	}
}



	
};