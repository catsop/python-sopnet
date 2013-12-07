
#include <iostream>
#include <string>
#include <fstream>
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
#include <sopnet/sopnet/block/Blocks.h>
#include <sopnet/sopnet/block/BlockManager.h>
#include <sopnet/sopnet/block/LocalBlockManager.h>
#include <imageprocessing/io/ImageBlockFileReader.h>
#include <util/point3.hpp>
#include <catmaidsopnet/SliceGuarantorParameters.h>
#include <catmaidsopnet/SliceGuarantor.h>
#include <catmaidsopnet/persistence/SliceStore.h>
#include <catmaidsopnet/persistence/LocalSliceStore.h>
#include <catmaidsopnet/persistence/SliceReader.h>


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

class InputPusher : public pipeline::SimpleProcessNode<>
{
private:
	pipeline::Input<Block> _in;
	pipeline::Output<Block> _out;
public:
	
	InputPusher()
	{
		registerInput(_in, "in");
		registerOutput(_out, "out");
	}
	
	void updateOutputs()
	{
		*_out = *_in;
	}
	
	void addInputTo(const boost::shared_ptr<ProcessNode>& node, const std::string& name)
	{
		node->addInput(name, _in);
	}
};

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
	std::string seriesDirectory = "/home/larry/smb/code/sopnet/data/testmembrane";
	boost::shared_ptr<BlockManager> blockManager = boost::make_shared<LocalBlockManager>(util::ptrTo(1024u, 1024u, 20u), util::ptrTo(256u, 256u, 2u));
	boost::shared_ptr<Block> block = boost::make_shared<Block>(id, util::ptrTo(0u,256u,0u), blockManager);
	boost::shared_ptr<pipeline::Wrap<unsigned int> > maxSize = boost::make_shared<pipeline::Wrap<unsigned int> >(256 * 256 * 64);
	
	
    try
    {
		LogManager::init();

		
		/*LOG_USER(out) << "Initing gui stuffs" << endl;
		
		// gui
        boost::shared_ptr<gui::Window> window = boost::make_shared<gui::Window>("larry");
        boost::shared_ptr<gui::ZoomView> zoomView = boost::make_shared<gui::ZoomView>();
        boost::shared_ptr<ContainerView<VerticalPlacing> > mainContainer = boost::make_shared<ContainerView<VerticalPlacing> >();
		boost::shared_ptr<ImageStackView> imageStackView = boost::make_shared<ImageStackView>();*/

		//data
		LOG_USER(out) << "Initting pipeline data" << endl;
		boost::shared_ptr<ImageBlockFactory> imageBlockFactory = boost::shared_ptr<ImageBlockFactory>(new ImageBlockFileFactory(seriesDirectory));
		boost::shared_ptr<pipeline::Wrap<bool> > nope = boost::make_shared<pipeline::Wrap<bool> >(false);
		boost::shared_ptr<SliceGuarantorParameters> params = boost::make_shared<SliceGuarantorParameters>();
		boost::shared_ptr<SliceStore> store = boost::shared_ptr<SliceStore>(new LocalSliceStore());
		boost::shared_ptr<SliceGuarantor> guarantor = boost::make_shared<SliceGuarantor>();
		pipeline::Value<Blocks> blocks;
		
		LOG_USER(out) << "Plugging the pipes into the other pipes" << endl;
		
		/*window->setInput(zoomView->getOutput());
		mainContainer->addInput(imageStackView->getOutput("painter"));
        zoomView->setInput(mainContainer->getOutput());*/

		params->guaranteeAllSlices = true;
		
		guarantor->setInput("block", block);
		guarantor->setInput("store", store);
		guarantor->setInput("block factory", imageBlockFactory);
		guarantor->setInput("force explanation", nope);
		guarantor->setInput("parameters", params);
		guarantor->setInput("maximum area", maxSize);

		LOG_USER(out) << "Guaranteeing some slices." << endl;
		blocks = guarantor->getOutput();;
		//LOG_USER(out) << "Guaranteed " << sliceWriteCount->count << " slices." << endl;
		LOG_USER(out) << "Found " << blocks->size() << " blocks to submit to fix border issues" << endl;
		
		LOG_USER(out) << "Let's read them back out" << endl;
		boost::shared_ptr<SliceReader> sliceReader= boost::make_shared<SliceReader>();
		pipeline::Value<Slices> slices;
		
		sliceReader->setInput("store", store);
		sliceReader->setInput("block", block);
		
		LOG_USER(out) << "Attempting to read slices" << endl;
		
		slices = sliceReader->getOutput();

		LOG_USER(out) << "Read " << slices->size() << " slices" << endl;
		
		std::ofstream sliceFile;
		sliceFile.open("slices.txt");

		LOG_USER(out) << "Writing slices to a text file in Matlab format. Slices are [-1 -1] terminated." << endl;
		
		foreach (boost::shared_ptr<Slice> slice, *slices)
		{
			double value = slice->getComponent()->getValue() - 1;
			foreach (const util::point<unsigned int>& pt, (slice->getComponent()->getPixels()))
			{
				sliceFile << pt.x << ", " << pt.y << ", " << value << ";" << std::endl;
			}
			sliceFile << " -1, -1, -2;" << std::endl;
		}
		
		LOG_USER(out) << "Donesies." << endl;
		
		sliceFile.close();
		
		
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
