
#include <iostream>
#include <string>
#include <fstream>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <util/Logger.h>

#include <pipeline/all.h>
#include <pipeline/Value.h>
#include <pipeline/Process.h>

#include <boost/unordered_set.hpp>
#include <util/ProgramOptions.h>
#include <util/exceptions.h>
#include <util/httpclient.h>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <sopnet/catmaid/persistence/django/DjangoBlockManager.h>
#include <tests/BlockManagerTest.h>
#include <tests/DjangoTestSuite.h>
#include <tests/LocalTestSuite.h>
#include <tests/CatsopTest.h>
#include <catmaid/persistence/django/CatmaidStackStore.h>
#include <imageprocessing/gui/ImageStackView.h>
#include <gui/Window.h>
#include <gui/ZoomView.h>

using std::cout;
using std::endl;
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
	util::ProgramOptions::init(optionc, optionv);
	LogManager::init();
	
	try
	{
// 		boost::shared_ptr<catsoptest::TestSuite> djangoSuite =
// 			catsoptest::DjangoTestSuite::djangoTestSuite("catmaid:8000", 4, 3);
// 		djangoSuite->runAll();
		boost::shared_ptr<catsoptest::TestSuite> localSuite = 
			catsoptest::LocalTestSuite::localTestSuite(util::point3<unsigned int>(179, 168, 5));
		localSuite->runAll();
// 		boost::shared_ptr<StackStore> stackStore = boost::make_shared<CatmaidStackStore>("catmaid:8000", 1, 1);
// 		boost::shared_ptr<Box<> > box = boost::make_shared<Box<> >(util::point3<unsigned int>(0,0,0),
// 																   util::point3<unsigned int>(1000,1000,5));
// 		boost::shared_ptr<ImageStack> stack = stackStore->getImageStack(*box);
// 		
// 		pipeline::Process<ImageStackView> stackView;
// 		pipeline::Process<gui::ZoomView> zoomView;
// 		pipeline::Process<gui::Window> window("yay");
// 		
// 		stackView->setInput(stack);
// 		zoomView->setInput(stackView->getOutput());
// 		window->setInput(zoomView->getOutput());
// 		
// 		window->processEvents();
		
	}
	catch (Exception& e)
	{
		handleException(e);
	}
}
