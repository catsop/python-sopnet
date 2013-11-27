
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
#include <util/exceptions.h>
#include <imageprocessing/gui/ImageView.h>
#include <imageprocessing/gui/ImageStackView.h>
#include <imageprocessing/io/ImageHttpReader.h>
#include <imageprocessing/io/ImageFileReader.h>
#include <imageprocessing/io/ImageBlockStackReader.h>
#include <util/ProgramOptions.h>
#include <ImageMagick/Magick++.h>
#include <sopnet/sopnet/block/Block.h>
#include <sopnet/sopnet/block/BlockManager.h>
#include <sopnet/sopnet/block/LocalBlockManager.h>
#include <imageprocessing/io/ImageBlockFileReader.h>
#include <util/point3.hpp>
#include <catmaidsopnet/SliceGuarantorParameters.h>
#include <catmaidsopnet/SliceGuarantor.h>
#include <catmaidsopnet/SliceStore.h>
#include <catmaidsopnet/LocalSliceStore.h>


using util::point3;
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
	std::string seriesDirectory = "/nfs/data0/home/larry/code/sopnet/data/testmembrane";
	boost::shared_ptr<BlockManager> blockManager = boost::make_shared<LocalBlockManager>(util::ptrTo(1024, 1024, 20), util::ptrTo(256, 256, 2));
	boost::shared_ptr<Block> block = boost::make_shared<Block>(id, util::ptrTo(0,0,0), blockManager);
	
    try
    {
		LogManager::init();

		
		/*LOG_USER(out) << "Initing gui stuffs" << endl;
		
		// gui
        boost::shared_ptr<gui::Window> window = boost::make_shared<gui::Window>("larry");
        boost::shared_ptr<gui::ZoomView> zoomView = boost::make_shared<gui::ZoomView>();
        boost::shared_ptr<ContainerView<VerticalPlacing> > mainContainer = boost::make_shared<ContainerView<VerticalPlacing> >();
		boost::shared_ptr<ImageStackView> imageStackView = boost::make_shared<ImageStackView>();*/

		LOG_USER(out) << "Initing data stuffs" << endl;
		
		//data
		LOG_USER(out) << "Initing ImageBlockFactory" << endl;
		boost::shared_ptr<ImageBlockFactory> imageBlockFactory = boost::shared_ptr<ImageBlockFactory>(new ImageBlockFileFactory(seriesDirectory));
		LOG_USER(out) << "Initing Nope" << endl;
		boost::shared_ptr<pipeline::Wrap<bool> > nope = boost::make_shared<pipeline::Wrap<bool> >(false);
		LOG_USER(out) << "Initing Params" << endl;
		boost::shared_ptr<SliceGuarantorParameters> params = boost::make_shared<SliceGuarantorParameters>();
		LOG_USER(out) << "Initing SliceStore" << endl;
		boost::shared_ptr<SliceStore> store = boost::shared_ptr<SliceStore>(new LocalSliceStore());
		LOG_USER(out) << "Initing SliceGuarantor" << endl;
		boost::shared_ptr<SliceGuarantor> guarantor = boost::make_shared<SliceGuarantor>();
		
		LOG_USER(out) << "Setting up pipeline inputs and such" << endl;
		
		/*window->setInput(zoomView->getOutput());
		mainContainer->addInput(imageStackView->getOutput("painter"));
        zoomView->setInput(mainContainer->getOutput());*/

		params->guaranteeAllSlices = true;
		
		guarantor->setInput("block", block);
		guarantor->setInput("store", store);
		guarantor->setInput("block factory", imageBlockFactory);
		guarantor->setInput("force explanation", nope);
		guarantor->setInput("parameters", params);

		LOG_USER(out) << "Guaranteeing some slices." << endl;
		guarantor->guaranteeSlices();
		LOG_USER(out) << "Guaranteed some slices." << endl;
		
        /*window->processEvents();
        while(!window->closed())
        {
			window->processEvents();
			usleep(1000);
		}*/
	}
    catch (Exception& e)
    {
		handleException(e);
	}
}
