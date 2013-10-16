
#include <iostream>
#include <string>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include <boost/progress.hpp>

#include <util/exceptions.h>
#include <gui/ContainerView.h>
#include <gui/Window.h>
#include <gui/ZoomView.h>
#include <util/exceptions.h>
#include <gui/VerticalPlacing.h>
#include <imageprocessing/gui/ImageView.h>
#include <imageprocessing/io/ImageReader.h>
#include <util/ProgramOptions.h>
#include <util/httpclient.h>

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
	
	HttpClient::response res = HttpClient::get("http://www.google.com");
	
	util::ProgramOptions::init(optionc, optionv);
	std::string fileName = "/home/larry/Images/Series/VolumeJosef/test.tiff";
	
    try
    {
		LogManager::init();
		        
        boost::shared_ptr<gui::Window> window = boost::make_shared<gui::Window>("larry");

        boost::shared_ptr<gui::ZoomView> zoomView = boost::make_shared<gui::ZoomView>();
        window->setInput(zoomView->getOutput());

        boost::shared_ptr<ContainerView<VerticalPlacing> > mainContainer = boost::make_shared<ContainerView<VerticalPlacing> >();

        boost::shared_ptr<ImageView> imageView = boost::make_shared<ImageView>();

        boost::shared_ptr<ImageReader> imageReader = boost::make_shared<ImageReader>(fileName.c_str());

        mainContainer->addInput(imageView->getOutput("painter"));
        zoomView->setInput(mainContainer->getOutput());

        imageView->setInput("image", imageReader->getOutput());
        
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
