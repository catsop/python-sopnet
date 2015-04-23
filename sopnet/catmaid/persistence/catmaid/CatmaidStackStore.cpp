#include "CatmaidStackStore.h"
#include <imageprocessing/io/ImageHttpReader.h>
#include <catmaid/persistence/django/DjangoUtils.h>
#include <util/exceptions.h>
#include <util/Logger.h>

logger::LogChannel catmaidstackstorelog("catmaidstackstorelog", "[CatmaidStackStore] ");


CatmaidStackStore::CatmaidStackStore(
		const ProjectConfiguration& configuration,
		StackType stackType) :
	_serverUrl(configuration.getCatmaidHost()),
	_project(configuration.getCatmaidProjectId()),
	_stack(configuration.getCatmaidStackIds(stackType).id),
	_stackScale(configuration.getCatmaidStackScale()),
	_treatMissingAsEmpty(stackType != Raw)
{
	boost::shared_ptr<ptree> pt;
	std::ostringstream os;
	DjangoUtils::appendProjectAndStack(os, _serverUrl, _project, _stack);
	os << "/stack_info";
	
	pt = _client.getPropertyTree(os.str());
	
	DjangoUtils::checkDjangoError(pt, os.str());

	std::vector<unsigned int> tileSizeVector, stackSizeVector;
	
	_imageBase = pt->get_child("image_base").get_value<std::string>();
	_extension = pt->get_child("file_extension").get_value<std::string>();
	_tileSourceType = pt->get_child("tile_source_type").get_value<unsigned int>();

	_tileWidth = pt->get_child("tile_width").get_value<unsigned int>();
	_tileHeight = pt->get_child("tile_height").get_value<unsigned int>();

	// Adjust the width and height of the stack for the configured scale
	_stackWidth = pt->get_child("dimension").get_child("x").get_value<unsigned int>();
	_stackWidth >>= _stackScale;
	_stackHeight = pt->get_child("dimension").get_child("y").get_value<unsigned int>();
	_stackHeight >>= _stackScale;
	_stackDepth = pt->get_child("dimension").get_child("z").get_value<unsigned int>();;

	// Check if the reported stack size is larger than the expected one or
	// smaller than the expected one by more than one block:
	const util::point3<unsigned int>& configVolume = configuration.getVolumeSize();
	const util::point3<unsigned int>& blockSize = configuration.getBlockSize();
	if (_stackWidth > configVolume.x || _stackWidth + blockSize.x <= configVolume.x ||
		_stackHeight > configVolume.y || _stackHeight + blockSize.y <= configVolume.y ||
		_stackDepth > configVolume.z || _stackDepth + blockSize.z <= configVolume.z)
		UTIL_THROW_EXCEPTION(
				UsageError,
				"catmaid stack size (" << _stackWidth << "," << _stackHeight << "," << _stackDepth <<
				") does not match expected size " << configVolume);
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
		for (unsigned int c = tileCMin; c < tileCMax; ++c)
		{
			unsigned int tileWXmin = c * _tileWidth; // Upper left of the tile in world coords.
			unsigned int tileWYmin = r * _tileHeight;

			try
			{
				boost::shared_ptr<ImageHttpReader> reader =
					boost::make_shared<ImageHttpReader>(tileURL(c, r, section), _client);
				pipeline::Value<Image> image = reader->getOutput();

				// This must be in the try block as the exception is not thrown
				// by pipeline until image is dereferenced.
				copyImageInto(*image, *imageOut, tileWXmin, tileWYmin, bound);
			}
			catch (ImageMissing& e)
			{
				if (_treatMissingAsEmpty)
				{
					Image empty(_tileWidth, _tileHeight);
					copyImageInto(empty, *imageOut, tileWXmin, tileWYmin, bound);
				}
				else throw;
			}
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

	switch (_tileSourceType) {
		case 1:
			url << _imageBase << section << "/" << row << "_" << column << "_"
			    << _stackScale << "." << _extension;
			break;
		case 5:
			url << _imageBase << _stackScale << '/' << section << '/'
			    << row << '/' << column << '.' << _extension;
			break;
		default:
			UTIL_THROW_EXCEPTION(
				NotYetImplemented,
				"CATMAID stack " << _stack << " uses unimplemented tile source type " << _tileSourceType);
	}

	return url.str();
}
