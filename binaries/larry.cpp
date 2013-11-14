
#include <iostream>
#include <string>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include <boost/progress.hpp>

#include <pipeline/all.h>
#include <pipeline/Value.h>
#include <util/exceptions.h>
#include <gui/ContainerView.h>
#include <gui/Window.h>
#include <gui/ZoomView.h>
#include <util/exceptions.h>
#include <gui/VerticalPlacing.h>
#include <imageprocessing/gui/ImageView.h>
#include <imageprocessing/gui/ImageStackView.h>
#include <imageprocessing/io/ImageHttpReader.h>
#include <imageprocessing/io/ImageFileReader.h>
#include <imageprocessing/io/ImageBlockStackReader.h>
#include <util/ProgramOptions.h>
#include <ImageMagick/Magick++.h>
#include <catmaidsopnet/Block.h>
#include <imageprocessing/io/ImageBlockFileReader.h>

using std::cout;
using std::endl;
using namespace gui;
using namespace logger;

void handleException(boost::exception& e) {

	LOG_ERROR(out) << "[window thread] caught exception: ";

	if (boost::get_error_info<error_message>(e))
		LOG_ERROR(out) << *boost::get_error_info<error_message>(e);

	if (boost::get_error_info<stack_trace>(e))
		LOG_ERROR(out) << *boost::get_error_info<stack_trace>(e);

	LOG_ERROR(out) << std::endl;

	LOG_ERROR(out) << "[window thread] details: " << std::endl
	               << boost::diagnostic_information(e)
	               << std::endl;

	exit(-1);
}

int main(int optionc, char** optionv)
{
	
	//HttpClient::response res = HttpClient::get("http://distilleryimage4.s3.amazonaws.com/5a32ffbc365a11e38b1f22000a9f135b_8.jpg");
	
	//LOG_USER(out) << "Response string " << res.body << std::endl;
	//Magick::Blob blob(res.body.c_str(), res.body.size());
	//Magick::Image image(blob);
	//image.magick("JPEG");
	//image.write("/home/larry/test.jpg");
	
	unsigned int id = 1;
	
	util::ProgramOptions::init(optionc, optionv);
	std::string fileName = "/nfs/data0/home/larry/Series/test.png";
	std::string url = "http://www.smbc-comics.com/comics/20131016.png";
	std::string seriesDirectory = "/nfs/data0/home/larry/Series/VolumeImages/";
	boost::shared_ptr<pipeline::Wrap<Block> > block = boost::make_shared<pipeline::Wrap<Block> >(Block(id, 0, 0, 16, 1024, 1024, 16));
	
    try
    {
		LogManager::init();
		        
        boost::shared_ptr<gui::Window> window = boost::make_shared<gui::Window>("larry");

        boost::shared_ptr<gui::ZoomView> zoomView = boost::make_shared<gui::ZoomView>();
        window->setInput(zoomView->getOutput());

        boost::shared_ptr<ContainerView<VerticalPlacing> > mainContainer = boost::make_shared<ContainerView<VerticalPlacing> >();

		boost::shared_ptr<ImageStackView> imageStackView = boost::make_shared<ImageStackView>();

		//boost::shared_ptr<ImageView> imageView = boost::make_shared<ImageView>();
		cout << "A" << std::endl;
		boost::shared_ptr<ImageBlockFileFactory> factory = boost::make_shared<ImageBlockFileFactory>(seriesDirectory);
		cout << "B" << std::endl;
		boost::shared_ptr<ImageBlockStackReader> stackReader = boost::make_shared<ImageBlockStackReader>(factory);
		cout << "C" << std::endl;
		
		//boost::shared_ptr<ImageBlockStackReader> stackReader =
		//	boost::make_shared<ImageBlockStackReader>(factory);

        //boost::shared_ptr<ImageHttpReader> imageReader = boost::make_shared<ImageHttpReader>(url);
		//boost::shared_ptr<ImageFileReader> imageReader = boost::make_shared<ImageFileReader>(fileName);
		//boost::shared_ptr<ImageBlockFileReader> imageReader = boost::make_shared<ImageBlockFileReader>(fileName, 0);
		//imageReader->setInput("block", block);
		stackReader->setInput("block", block);
		cout << "D" << std::endl;
		imageStackView->setInput(stackReader->getOutput("stack"));
		mainContainer->addInput(imageStackView->getOutput("painter"));
        //mainContainer->addInput(imageView->getOutput("painter"));
        zoomView->setInput(mainContainer->getOutput());
		
        //imageView->setInput("image", imageReader->getOutput());
		//pipeline::Value<ImageStack> stackValue;
		//pipeline::Value<ImageStack> output = stackReader->getOutput("stack");
		//*stackValue = *output;
        
		cout << "Yup. Got this far. Dammit." << std::endl;
		
        window->processEvents();
        while(!window->closed())
        {
			window->processEvents();
			usleep(1000);
		}
	}
    catch (Exception& e)
    {
		handleException(e);
	}
}
