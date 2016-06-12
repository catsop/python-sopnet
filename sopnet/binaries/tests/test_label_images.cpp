#include <cstdint>
#include <iomanip>
#include <iostream>
#include <random>

#include <boost/filesystem.hpp>

#include <imageprocessing/io/ImageFileReader.h>
#include <imageprocessing/io/ImageHttpReader.h>
#include <imageprocessing/io/ImageWriter.h>
#include <pipeline/Value.h>
#include <util/exceptions.h>
#include <util/Logger.h>

int main() {

	try {
		std::cout << "Generating random label image..." << std::endl;

		// init logger
		logger::LogManager::init();
		logger::LogManager::setGlobalLogLevel(logger::Debug);

		boost::shared_ptr<LabelImage> original = boost::make_shared<LabelImage>(1024, 1024);

		std::random_device rd;
		std::mt19937_64 gen(rd());

		/* This is where you define the number generator for unsigned long long: */
		std::uniform_int_distribution<uint64_t> dis;

		for (int i = 0; i < original->size(); ++i)
			(*original)[i] = dis(gen);

		const std::string TEST_FILENAME = "test_label_image.png";

		std::cout << "Writing label image..." << std::endl;
		boost::shared_ptr<ImageWriter<LabelImage> > writer = boost::make_shared<ImageWriter<LabelImage> >(TEST_FILENAME);
		writer->setInput("image", original);
		writer->write(TEST_FILENAME);
		
		std::cout << "Reading label image..." << std::endl;
		boost::shared_ptr<ImageFileReader<LabelImage> > reader = boost::make_shared<ImageFileReader<LabelImage> >(TEST_FILENAME);
		pipeline::Value<LabelImage> recovered = reader->getOutput("image");

		if (*original != *recovered) {
			UTIL_THROW_EXCEPTION(
					Exception,
					"Images differ.");
		}
		

		boost::filesystem::path full_path(boost::filesystem::current_path());
		std::ostringstream urlStream;
		urlStream << "file://" << full_path.string() << '/' << TEST_FILENAME;
		const std::string url = urlStream.str();

		std::cout << "Reading label image via CURL at " << url << std::endl;
		HttpClient client;
		boost::shared_ptr<ImageHttpReader<LabelImage> > httpReader = boost::make_shared<ImageHttpReader<LabelImage> >(url, client);
		pipeline::Value<LabelImage> httpRecovered = httpReader->getOutput("image");

		if (*original != *httpRecovered) {
			std::cout << std::hex << (*original)[100] << " " << std::hex << (*httpRecovered)[100] << std::endl;
			UTIL_THROW_EXCEPTION(
					Exception,
					"Images differ.");
		}

	} catch (boost::exception& e) {

		handleException(e, std::cerr);
	}
}
