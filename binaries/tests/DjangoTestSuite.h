#ifndef TEST_DJANGO_SUITE_H__
#define TEST_DJANGO_SUITE_H__
#include "BlockManagerTest.h"
#include "SliceStoreTest.h"
#include <sopnet/block/BlockManager.h>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

namespace catsoptest
{

class SliceStoreFactory;
class DjangoBlockManagerFactory : public BlockManagerFactory
{
public:
	DjangoBlockManagerFactory(const std::string server,
							  const unsigned int project,
							  const unsigned int stack);
	
	boost::shared_ptr<BlockManager> createBlockManager(
		const util::point3<unsigned int> blockSize,
		const util::point3<unsigned int> coreSizeInBlocks);
	
private:
	const std::string _server;
	const unsigned int _project, _stack;
};

class DjangoSliceStoreFactory : public SliceStoreFactory
{
public:
	DjangoSliceStoreFactory(const std::string& url, unsigned int project, unsigned int stack);
	boost::shared_ptr<SliceStore> createSliceStore();
private:
	const std::string _url;
	const unsigned int _project, _stack;
};

class DjangoTestSuite
{
public:
	static boost::shared_ptr<TestSuite> djangoTestSuite(const std::string& url,
		unsigned int project, unsigned int stack);
	
private:
	static void addBlockManagerTest(const boost::shared_ptr<TestSuite> suite,
		const std::string& url, unsigned int project, unsigned int stack);
	
	static void addSliceStoreTest(const boost::shared_ptr<TestSuite> suite,
		const std::string& url, unsigned int project, unsigned int stack);
};

};

#endif //TEST_DJANGO_SUITE_H__