#include "CatmaidStackStore.h"
#include <imageprocessing/io/ImageHttpReader.h>
#include <util/httpclient.h>
#include "DjangoUtils.h"
#include <util/Logger.h>

logger::LogChannel catmaidstackstorelog("catmaidstackstorelog", "[CatmaidStackStore] ");


CatmaidStackStore::CatmaidStackStore(const std::string& url,
									 unsigned int project,
									 unsigned int stack) :
									 _serverUrl(url), _project(project), _stack(stack)
{
	boost::shared_ptr<ptree> pt;
	std::ostringstream os;
	DjangoUtils::appendProjectAndStack(os, _serverUrl, _project, _stack);
	os << "/stack_info";
	
	pt = HttpClient::getPropertyTree(os.str());
	
	DjangoUtils::checkDjangoError(pt, os.str());

	std::vector<unsigned int> tileSizeVector, stackSizeVector;
	
	_imageBase = pt->get_child("image_base").get_value<std::string>();
	_extension = pt->get_child("file_extension").get_value<std::string>();
	
	HttpClient::ptreeVector<unsigned int>(pt->get_child("tile_size"), tileSizeVector);
	HttpClient::ptreeVector<unsigned int>(pt->get_child("stack_size"), stackSizeVector);
	
	_tileWidth = tileSizeVector[0];
	_tileHeight = tileSizeVector[1];
	
	_stackWidth = stackSizeVector[0];
	_stackHeight = stackSizeVector[1];
	_stackDepth = stackSizeVector[2];
	
	_ok = true;
}


boost::shared_ptr<Image>
CatmaidStackStore::getImage(const util::rect<unsigned int> bound,
							const unsigned int section)
{
	/*
	Step 1) Calculate which tiles we need to fetch. This is done by dividing the bounds by the
	        tile width and height.
	Step 2) Actually fetch those tiles, then concatenate them into a single image
	Step 3) Crop the image to the correct boundary size
	*/
	unsigned int tileCMin, tileCMax, tileRMin, tileRMax;
	std::vector<boost::shared_ptr<Image> > catImages;
	boost::shared_ptr<Image> imageOut = boost::make_shared<Image>(bound.width(), bound.height());
	
	tileCMin = bound.minX / _tileWidth;
	tileRMin = bound.minY / _tileHeight;
	// For the max, we want the integer division to round up to get exclusive 
	// (c,r)-max coordinates. We do that by adding the tilesize - 1 before 
	// dividing and rounding down.
	tileCMax = (bound.maxX + _tileWidth - 1) / _tileWidth;
	tileRMax = (bound.maxY + _tileHeight -1) / _tileHeight;
	
	for (unsigned int r = tileRMin; r < tileRMax; ++r)
	{
		std::vector<boost::shared_ptr<Image> > imageVector;
		for (unsigned int c = tileCMin; c < tileCMax; ++c)
		{
			boost::shared_ptr<ImageHttpReader> reader =
				boost::make_shared<ImageHttpReader>(tileURL(c, r, section));
			pipeline::Value<Image> image = reader->getOutput();

			unsigned int tileWXmin = c * _tileWidth; // Upper left of the tile in world coords.
			unsigned int tileWYmin = r * _tileHeight;

			copyImageInto(*image, *imageOut, tileWXmin, tileWYmin, bound);
		}
		
		
	}
	
	
	return imageOut;
}

void
CatmaidStackStore::copyImageInto(const Image& tile,
								 const Image& request,
								 const unsigned int tileWXmin,
								 const unsigned int tileWYmin,
								 const util::rect<unsigned int> bound)
{
	// beg, end refer to source tile image, dst refers to destination request region,
	// which belongs to the Image that we eventually return.
	Image::difference_type beg, end, dst;
	
	if (tileWXmin >= bound.minX)
	{
		beg[0] = 0;
		dst[0] = tileWXmin - bound.minX;
	}
	else
	{
		beg[0] = bound.minX - tileWXmin;
		dst[0] = 0;
	}
	
	if (tileWYmin >= bound.minY)
	{
		beg[1] = 0;
		dst[1] = tileWYmin - bound.minY;
	}
	else
	{
		beg[1] = bound.minY - tileWYmin;
		dst[1] = 0;
	}
	
	if (tileWXmin + _tileWidth <= bound.maxX)
	{
		end[0] = _tileWidth;
	}
	else
	{
		end[0] = bound.maxX - tileWXmin;
	}
	
	if (tileWYmin + _tileHeight <= bound.maxY)
	{
		end[1] = _tileHeight;
	}
	else
	{
		end[1] = bound.maxY - tileWYmin;
	}
	
	vigra::copyImage(tile.subarray(beg, end), request.subarray(dst, dst + end - beg));
}


std::string
CatmaidStackStore::tileURL(const unsigned int column, const unsigned int row, const unsigned int section)
{
	std::stringstream url;
	url << _imageBase << section << "/" << row << "_" << column << "_0." << _extension;
	return url.str();
}
