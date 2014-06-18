#ifndef CATMAID_STACK_STORE_H__
#define CATMAID_STACK_STORE_H__
#include <catmaid/persistence/StackStore.h>

/*
 * Catmaid-backed stack store
 */
class CatmaidStackStore : public StackStore
{
public:
	CatmaidStackStore(const std::string& url, 
					  unsigned int project,
					  unsigned int stack);
protected:
	boost::shared_ptr<Image> getImage(const util::rect<unsigned int> bound,
									  const unsigned int section);
private:
	std::string tileURL(const unsigned int column, const unsigned int row,
						const unsigned int section);
	
	void copyImageInto(const Image& tile,
					   const Image& request,
					   const unsigned int tileWXmin,
					   const unsigned int tileWYmin,
					   const util::rect<unsigned int> bound);
	
	const std::string _serverUrl;
	const unsigned int _project, _stack;
	std::string _imageBase, _extension;
	unsigned int _tileWidth, _tileHeight, _stackWidth, _stackHeight, _stackDepth;
	bool _ok;
};


#endif //CATMAID_STACK_STORE_H__