#include "CatmaidStackStore.h"
#include <imageprocessing/io/ImageHttpReader.h>
#include <util/exceptions.h>
#include <util/Logger.h>

logger::LogChannel catmaidstackstorelog("catmaidstackstorelog", "[CatmaidStackStore] ");


CatmaidStackStore::CatmaidStackStore(
		const ProjectConfiguration& configuration,
		StackType stackType) :
	_stack(configuration.getCatmaidStack(stackType)),
	_treatMissingAsEmpty(stackType != Raw)
{

	// Adjust the width and height of the stack for the configured scale
	unsigned int scaledWidth, scaledHeight;
	scaledWidth = _stack.width;
	scaledWidth >>= _stack.scale;
	scaledHeight = _stack.height;
	scaledHeight >>= _stack.scale;

	// Check if the reported stack size is larger than the expected one or
	// smaller than the expected one by more than one block:
	const util::point<unsigned int, 3>& configVolume = configuration.getVolumeSize();
	const util::point<unsigned int, 3>& blockSize = configuration.getBlockSize();
	if (scaledWidth > configVolume.x() || scaledWidth + blockSize.x() <= configVolume.x() ||
		scaledHeight > configVolume.y() || scaledHeight + blockSize.y() <= configVolume.y() ||
		_stack.depth > configVolume.z() || _stack.depth + blockSize.z() <= configVolume.z())
		UTIL_THROW_EXCEPTION(
				UsageError,
				"Scaled catmaid stack size (" << scaledWidth << "," << scaledHeight << "," << _stack.depth <<
				") does not match expected size " << configVolume);
}


boost::shared_ptr<Image>
CatmaidStackStore::getImage(const util::box<unsigned int, 2> bound,
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
	
	tileCMin = bound.min().x() / _stack.tileWidth;
	tileRMin = bound.min().y() / _stack.tileHeight;
	// For the max, we want the integer division to round up to get exclusive 
	// (c,r)-max coordinates. We do that by adding the tilesize - 1 before 
	// dividing and rounding down.
	tileCMax = (bound.max().x() + _stack.tileWidth - 1) / _stack.tileWidth;
	tileRMax = (bound.max().y() + _stack.tileHeight -1) / _stack.tileHeight;
	
	for (unsigned int r = tileRMin; r < tileRMax; ++r)
	{
		for (unsigned int c = tileCMin; c < tileCMax; ++c)
		{
			unsigned int tileWXmin = c * _stack.tileWidth; // Upper left of the tile in world coords.
			unsigned int tileWYmin = r * _stack.tileHeight;

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
					Image empty(_stack.tileWidth, _stack.tileHeight);
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
								 const util::box<unsigned int, 2> bound)
{
	// beg, end refer to source tile image, dst refers to destination request region,
	// which belongs to the Image that we eventually return.
	Image::difference_type beg, end, dst;
	
	if (tileWXmin >= bound.min().x())
	{
		beg[0] = 0;
		dst[0] = tileWXmin - bound.min().x();
	}
	else
	{
		beg[0] = bound.min().x() - tileWXmin;
		dst[0] = 0;
	}
	
	if (tileWYmin >= bound.min().y())
	{
		beg[1] = 0;
		dst[1] = tileWYmin - bound.min().y();
	}
	else
	{
		beg[1] = bound.min().y() - tileWYmin;
		dst[1] = 0;
	}
	
	if (tileWXmin + _stack.tileWidth <= bound.max().x())
	{
		end[0] = _stack.tileWidth;
	}
	else
	{
		end[0] = bound.max().x() - tileWXmin;
	}
	
	if (tileWYmin + _stack.tileHeight <= bound.max().y())
	{
		end[1] = _stack.tileHeight;
	}
	else
	{
		end[1] = bound.max().y() - tileWYmin;
	}
	
	vigra::copyImage(tile.subarray(beg, end), request.subarray(dst, dst + end - beg));
}


std::string
CatmaidStackStore::tileURL(const unsigned int column, const unsigned int row, const unsigned int section)
{
	std::stringstream url;

	switch (_stack.tileSourceType) {
		case 1:
			url << _stack.imageBase << section << "/" << row << "_" << column << "_"
			    << _stack.scale << "." << _stack.fileExtension;
			break;
		case 5:
			url << _stack.imageBase << _stack.scale << '/' << section << '/'
			    << row << '/' << column << '.' << _stack.fileExtension;
			break;
		default:
			UTIL_THROW_EXCEPTION(
				NotYetImplemented,
				"CATMAID stack " << _stack.id << " uses unimplemented tile source type " << _stack.tileSourceType);
	}

	return url.str();
}
