#ifndef CATMAID_STACK_STORE_H__
#define CATMAID_STACK_STORE_H__
#include <catmaid/ProjectConfiguration.h>
#include <catmaid/persistence/StackStore.h>
#include <catmaid/persistence/StackType.h>

/*
 * Catmaid-backed stack store
 */
class CatmaidStackStore : public StackStore
{
public:

	/**
	 * Construct a CatmaidStackStore. Uses the catmaid server that was set in 
	 * the given project configuration.
	 */
	CatmaidStackStore(const ProjectConfiguration& configuration, StackType stackType);

private:
	boost::shared_ptr<Image> getImage(const util::rect<unsigned int> bound,
									  const unsigned int section);

	/**
	 * Helper function to grab the URL for the tile at the given column, row,
	 * and section. Uses the stack scale from ProjectConfiguration. Currently
	 * supports CATMAID tile source types 1 and 5.
	 */
	std::string tileURL(const unsigned int column, const unsigned int row,
						const unsigned int section);
	
	/**
	 * Copies the tile image into the output image.
	 * @param tile - the Image returned for the given CATMAID tile
	 * @param request - the Image to be returned, representing the requested Image.
	 * @param tileWXmin - the x-coordinate of corner of the tile closest to the origin, in world
	 *                    (ie, stack) coordinates.
	 * @param tileWYmin - the y-coordinate --"--
	 * @param bound - a rect representing the bounding box for the requested Image in world
	 *                (again, stack) coordinates.
	 */
	void copyImageInto(const Image& tile,
					   const Image& request,
					   const unsigned int tileWXmin,
					   const unsigned int tileWYmin,
					   const util::rect<unsigned int> bound);
	
	const std::string _serverUrl;
	const unsigned int _project, _stack;
	std::string _imageBase, _extension;
	unsigned int _tileSourceType, _tileWidth, _tileHeight, _stackWidth, _stackHeight, _stackDepth;
	const unsigned int _stackScale;
	bool _ok;
};


#endif //CATMAID_STACK_STORE_H__
