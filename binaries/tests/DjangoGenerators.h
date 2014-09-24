#ifndef TEST_DJANGO_GENERATORS_H__
#define TEST_DJANGO_GENERATORS_H__

#include <string>
#include <catmaid/persistence/BlockManager.h>
#include <catmaid/persistence/django/DjangoBlockManager.h>

namespace catsoptest
{
	bool clearDJSopnet(const std::string& server, const unsigned int project,
					   const unsigned int stack);
	
	boost::shared_ptr<DjangoBlockManager> getNewDjangoBlockManager(
		const std::string& server, const unsigned int project, const unsigned int stack,
		const util::point3<unsigned int> blockSize,
		const util::point3<unsigned int> coreSizeInBlocks);
};

#endif //TEST_DJANGO_GENERATORS_H__
