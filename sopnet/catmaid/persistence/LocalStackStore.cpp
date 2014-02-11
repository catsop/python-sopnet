#include "LocalStackStore.h"

#include <imageprocessing/io/ImageFileReader.h>
#include <imageprocessing/ImageCrop.h>
#include <util/Logger.h>

static logger::LogChannel localstackstorelog("localstackstorelog", "[LocalStackStore] ");


LocalStackStore::LocalStackStore(std::string directory) 
{
	LOG_DEBUG(localstackstorelog) << "reading from directory " << directory << std::endl;

	boost::filesystem::path dir(directory);

	if (!boost::filesystem::exists(dir))
	{
		BOOST_THROW_EXCEPTION(IOError() << error_message(directory + " does not exist"));
	}

	if (!boost::filesystem::is_directory(dir))
	{
		BOOST_THROW_EXCEPTION(IOError() << error_message(directory + " is not a directory"));
	}

	std::copy(
			boost::filesystem::directory_iterator(dir),
			boost::filesystem::directory_iterator(),
			back_inserter(_imagePaths));
	std::sort(_imagePaths.begin(), _imagePaths.end());

	LOG_DEBUG(localstackstorelog) << "directory contains " << _imagePaths.size() <<
		" entries" << std::endl;

}

boost::shared_ptr<Image> LocalStackStore::getImage(util::rect<unsigned int> bound,
												   unsigned int section)
{
	if (section < _imagePaths.size())
	{
		boost::filesystem::path file = _imagePaths[section];
		boost::shared_ptr<ImageFileReader> reader = boost::make_shared<ImageFileReader>(file.c_str());
		boost::shared_ptr<ImageCrop> cropper = boost::make_shared<ImageCrop>();
		pipeline::Value<Image> croppedImage, image;
		pipeline::Value<int> x(bound.minX), y(bound.minY),
							 w(bound.maxX - bound.minX), h(bound.maxY - bound.minY);

		LOG_ALL(localstackstorelog) << "Reading image from " << file << std::endl;
		// check bounds
		image = reader->getOutput("image");
		
		if (image->width() < *x || image->height() < *y)
		{
			LOG_DEBUG(localstackstorelog) << "Image does not overlap block. Image of size " <<
				image->width() << "x" << image->height() << ", bound: " << bound << std::endl;
			return boost::make_shared<Image>();
		}
		
		if (image->width() < *x + *w || image->height() < *y + *h)
		{
			*w = image->width() - *x;
			*h = image->height() - *y;
			LOG_ALL(localstackstorelog) << "Bound " << bound <<
				" did not fit inside image with size " <<
				image->width() << "x" << image->height() << std::endl;
				
		}
		
		cropper->setInput("image", reader->getOutput("image"));
		cropper->setInput("x", x);
		cropper->setInput("y", y);
		cropper->setInput("width", w);
		cropper->setInput("height", h);
		
		croppedImage = cropper->getOutput("cropped image");
		
		return croppedImage;
	}
	else
	{
		LOG_DEBUG(localstackstorelog) << "Requested section " << section <<
			" does not have a corresponding image." << std::endl;
		return boost::make_shared<Image>();
	}
}
