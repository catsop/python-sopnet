#include "DjangoTestSuite.h"
#include <binaries/tests/DjangoGenerators.h>
#include <catmaid/django/DjangoUtils.h>

#include <util/Logger.h>

logger::LogChannel djangotestlog("djangotestlog", "[DjangoTestSuite] ");

namespace catsoptest
{

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


boost::shared_ptr<TestSuite>
DjangoTestSuite::djangoTestSuite(const std::string& url, unsigned int project, unsigned int stack)
{
	LOG_USER(djangotestlog) << "Running django test suite" << std::endl;
	boost::shared_ptr<TestSuite> suite = boost::make_shared<TestSuite>("Django");
	util::point3<unsigned int> stackSize = *DjangoUtils::getStackSize(url, project, stack);
	boost::shared_ptr<BlockManagerFactory> factory =
		boost::make_shared<DjangoBlockManagerFactory>(url, project, stack);
	
	boost::shared_ptr<Test<BlockManagerTestParam> > test =
		boost::make_shared<BlockManagerTest>(factory);
	
	LOG_USER(djangotestlog) << "For server " << url << ", project " << project << ", and stack "
		<< stack << ", got stack size: " << stackSize << std::endl;
		
	if (stackSize > util::point3<unsigned int>(0,0,0))
	{
		suite->addTest<BlockManagerTestParam>(
			test, BlockManagerTest::generateTestParameters(stackSize));
	}
	
	return suite;
}

	
};