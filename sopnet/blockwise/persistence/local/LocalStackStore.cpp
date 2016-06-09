#include "LocalStackStore.h"

#include <imageprocessing/io/ImageFileReader.h>
#include <imageprocessing/ImageCrop.h>
#include <util/Logger.h>

static logger::LogChannel localstackstorelog("localstackstorelog", "[LocalStackStore] ");


template <typename ImageType>
LocalStackStore<ImageType>::LocalStackStore(std::string directory) 
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

template <typename ImageType>
boost::shared_ptr<ImageType> LocalStackStore<ImageType>::getImage(util::box<unsigned int, 2> bound,
												                  unsigned int section)
{
	if (section < _imagePaths.size())
	{
		boost::filesystem::path file = _imagePaths[section];
		boost::shared_ptr<ImageFileReader<ImageType> > reader = boost::make_shared<ImageFileReader<ImageType> >(file.c_str());
		boost::shared_ptr<ImageCrop<ImageType> > cropper = boost::make_shared<ImageCrop<ImageType> >();
		pipeline::Value<ImageType> croppedImage, image;
		pipeline::Value<int> x(bound.min().x()), y(bound.min().y()),
							 w(bound.max().x() - bound.min().x()), h(bound.max().y() - bound.min().y());

		LOG_ALL(localstackstorelog) << "Reading image from " << file << std::endl;
		// check bounds
		image = reader->getOutput("image");
		
		if (image->width() < *x || image->height() < *y)
		{
			LOG_DEBUG(localstackstorelog) << "Image does not overlap block. Image of size " <<
				image->width() << "x" << image->height() << ", bound: " << bound << std::endl;
			return boost::make_shared<ImageType>();
		}
		
		if (image->width() < *x + *w || image->height() < *y + *h)
		{
			*w = image->width() - *x;
			*h = image->height() - *y;
			LOG_DEBUG(localstackstorelog) << "Bound " << bound <<
				" did not fit inside image with size " <<
				image->width() << "x" << image->height() << std::endl;
				
		}
		
		cropper->setInput("image", reader->getOutput("image"));
		cropper->setInput("x", x);
		cropper->setInput("y", y);
		cropper->setInput("width", w);
		cropper->setInput("height", h);
		
		croppedImage = cropper->getOutput("cropped image");
		
		LOG_ALL(localstackstorelog) << "Returning cropped image of size" << 
			croppedImage->width() << "x" << croppedImage->height() << std::endl;
		
		return croppedImage;
	}
	else
	{
		LOG_DEBUG(localstackstorelog) << "Requested section " << section <<
			" does not have a corresponding image." << std::endl;
		return boost::make_shared<ImageType>();
	}
}

template class LocalStackStore<IntensityImage>;
