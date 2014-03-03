#include <string>
#include <fstream>
#include <iostream>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem.hpp>

#include <imageprocessing/ImageStack.h>

#include <util/point3.hpp>
#include <util/ProgramOptions.h>
#include <pipeline/all.h>
#include <pipeline/Value.h>
#include <pipeline/Process.h>
#include <sopnet/Sopnet.h>
#include <sopnet/inference/PriorCostFunctionParameters.h>
#include <sopnet/inference/SegmentationCostFunctionParameters.h>
#include <sopnet/segments/Segments.h>
#include <sopnet/block/Block.h>
#include <sopnet/block/Blocks.h>
#include <sopnet/block/BlockManager.h>
#include <sopnet/block/Box.h>
#include <sopnet/block/LocalBlockManager.h>
#include <imageprocessing/io/ImageStackDirectoryReader.h>
#include <imageprocessing/io/ImageBlockFactory.h>
#include <imageprocessing/io/ImageBlockFileReader.h>
#include <catmaid/CoreSolver.h>
#include <catmaid/SegmentGuarantor.h>
#include <catmaid/SliceGuarantor.h>
#include <catmaid/persistence/LocalSegmentStore.h>
#include <catmaid/persistence/LocalSliceStore.h>
#include <catmaid/persistence/LocalStackStore.h>
#include <catmaid/persistence/SegmentReader.h>
#include <sopnet/segments/SegmentSet.h>

#include <sopnet/block/Box.h>
#include <vigra/impex.hxx>
#include <sopnet/slices/SliceExtractor.h>
#include <gui/Window.h>
#include <imageprocessing/gui/ImageStackView.h>
#include <neurons/NeuronExtractor.h>

#include <util/Logger.h>

using util::point3;
using namespace logger;
using namespace std;

//logger::LogChannel coretestlog("coretestlog", "[CoreTest] ");

util::ProgramOption optionCoreTestMembranesPath(
	util::_module = 			"core",
	util::_long_name = 			"membranes",
	util::_description_text = 	"Path to membrane image stack",
	util::_default_value =		"./membranes");
	
util::ProgramOption optionCoreTestRawImagesPath(
	util::_module = 			"core",
	util::_long_name = 			"raw",
	util::_description_text = 	"Path to raw image stack",
	util::_default_value =		"./raw");

util::ProgramOption optionCoreTestBlockSizeFraction(
	util::_module = 			"core",
	util::_long_name = 			"blockSizeFraction",
	util::_description_text = 	"Block size, as a fraction of the whole stack, like numerator/denominator",
	util::_default_value =		"2/5");

util::ProgramOption optionCoreTestSequential(
	util::_module = 			"core",
	util::_long_name = 			"sequential",
	util::_description_text = 	"Determines whether to extract slices/segments sequentially or simultaneously",
	util::_default_value =		true);

util::ProgramOption optionCoreTestForceExplanation(
	util::_module = 			"core",
	util::_long_name = 			"forceExplanation",
	util::_description_text = 	"Force Explanation",
	util::_default_value =		true);

util::ProgramOption optionCoreTestWriteSliceImages(
	util::_module = 			"core",
	util::_long_name = 			"writeSliceImages",
	util::_description_text = 	"Write slice images");

util::ProgramOption optionCoreTestWriteDebugFiles(
	util::_module = 			"core",
	util::_long_name = 			"writeDebugFiles",
	util::_description_text = 	"Write debug files");

std::string sopnetOutputPath = "./out-sopnet";
std::string blockwiseOutputPath = "./out-blockwise";

int experiment = 0;

void handleException(boost::exception& e) {

	LOG_ERROR(out) << "caught exception: ";

	if (boost::get_error_info<error_message>(e))
		LOG_ERROR(out) << *boost::get_error_info<error_message>(e);

	if (boost::get_error_info<stack_trace>(e))
		LOG_ERROR(out) << *boost::get_error_info<stack_trace>(e);

	LOG_ERROR(out) << std::endl;

	LOG_ERROR(out) << "details: " << std::endl
	               << boost::diagnostic_information(e)
	               << std::endl;

	exit(-1);
}

void writeConflictSets(const boost::shared_ptr<ConflictSets> conflictSets,
					   const std::string& path)
{
	ofstream conflictFile;
	std::string conflictFilePath = path + "/conflict.txt";
	conflictFile.open(conflictFilePath.c_str());
	
	foreach (const ConflictSet conflictSet, *conflictSets)
	{
		foreach (unsigned int id, conflictSet.getSlices())
		{
			conflictFile << id << " ";
		}
		conflictFile << endl;
	}
	
	conflictFile.close();
	
}

void writeSlice(const Slice& slice, const std::string& sliceImageDirectory, ofstream& file) {

	unsigned int section = slice.getSection();
	unsigned int id      = slice.getId();
	util::rect<int> bbox = slice.getComponent()->getBoundingBox();

	std::string filename = sliceImageDirectory + "/" + boost::lexical_cast<std::string>(section) +
		"_" + boost::lexical_cast<std::string>(id) + "_" + boost::lexical_cast<std::string>(bbox.minX) +
		"_" + boost::lexical_cast<std::string>(bbox.minY) + ".png";

	if (optionCoreTestWriteDebugFiles)
	{
		file << id << ", " << slice.hashValue() << ";" << endl;
	}

	if (optionCoreTestWriteSliceImages)
	{
		vigra::exportImage(vigra::srcImageRange(slice.getComponent()->getBitmap()),
						vigra::ImageExportInfo(filename.c_str()));
	}
}

void writeSlices(const boost::shared_ptr<Slices> slices,
					  const std::string path)
{
	boost::filesystem::path dir(path);
	
	ofstream sliceFile;
	string slicelog = path + "/sliceids.txt";

	if (!optionCoreTestWriteDebugFiles && !optionCoreTestWriteSliceImages)
	{
		return;
	}
	
	if (optionCoreTestWriteDebugFiles)
	{
		sliceFile.open(slicelog.c_str());
	}

	
	foreach (boost::shared_ptr<Slice>& slice, *slices)
	{
		writeSlice(*slice, path, sliceFile);
	}
	
	if (optionCoreTestWriteDebugFiles)
	{
		sliceFile.close();
	}
}

unsigned int fractionCeiling(unsigned int m, unsigned int num, unsigned int denom)
{
	unsigned int result = m * num / denom;
	if (result * denom < m * num)
	{	
		result += 1;
	}
	return result;
}

void writeSegment(ofstream& file, const boost::shared_ptr<Segment> segment)
{
	int i = 0;
	
	switch (segment->getType())
	{
		case EndSegmentType:
			file << "0 ";
			break;
		case ContinuationSegmentType:
			file << "1 ";
			break;
		case BranchSegmentType:
			file << "2 ";
			break;
		default:
			file << "-1 ";
	}
	
	foreach (boost::shared_ptr<Slice> slice, segment->getSlices())
	{
		++i;
		file << slice->hashValue() << " ";
	}
	
	for (int j = i; j < 3; ++j)
	{
		file << "0 ";
	}
	
	file << endl;
}

void writeSegments(const boost::shared_ptr<Segments> segments,
				   const std::string path)
{
	boost::filesystem::path dir(path);
	
	ofstream segmentFile;
	std::string segmentlog = path + "/segments.txt";

	segmentFile.open(segmentlog.c_str());
	
	foreach (boost::shared_ptr<Segment> segment, segments->getSegments())
	{
		writeSegment(segmentFile, segment);
	}

}

int sliceContains(const boost::shared_ptr<Slices> slices,
				   const boost::shared_ptr<Slice> slice)
{
	foreach (boost::shared_ptr<Slice> otherSlice, *slices)
	{
		if (*otherSlice == *slice)
		{
			return otherSlice->getId();
		}
	}
	
	return -1;
}

boost::shared_ptr<ConflictSets> mapConflictSets(const boost::shared_ptr<ConflictSets> conflictSets,
												std::map<unsigned int, unsigned int>& idMap)
{
	boost::shared_ptr<ConflictSets> mappedSets = boost::make_shared<ConflictSets>();
	foreach (ConflictSet conflictSet, *conflictSets)
	{
		ConflictSet mappedSet;
		
		foreach (unsigned int id, conflictSet.getSlices())
		{
			mappedSet.addSlice(idMap[id]);
		}
		
		mappedSets->add(mappedSet);
	}
	
	return mappedSets;
}

bool conflictSetContains(const boost::shared_ptr<ConflictSets> conflictSets,
						 const ConflictSet conflictSet)
{
	foreach (ConflictSet otherConflictSet, *conflictSets)
	{
		if (otherConflictSet == conflictSet)
		{
			return true;
		}
	}
	
	return false;
}

bool segmentsContains(const boost::shared_ptr<Segments> segments,
					  const boost::shared_ptr<Segment> segment)
{
	foreach (boost::shared_ptr<Segment> otherSegment, segments->getSegments())
	{
		if (*otherSegment == *segment)
		{
			return true;
		}
	}
	
	return false;
}

bool testSlices(util::point3<unsigned int> stackSize, util::point3<unsigned int> blockSize)
{
	// SOPNET variables
	int i = 0;
	std::string membranePath = optionCoreTestMembranesPath.as<std::string>();
	
	boost::shared_ptr<ImageStackDirectoryReader> directoryStackReader =
			boost::make_shared<ImageStackDirectoryReader>(membranePath);
	pipeline::Value<ImageStack> stack = directoryStackReader->getOutput();
	
	pipeline::Value<Slices> sopnetSlices = pipeline::Value<Slices>();
	pipeline::Value<ConflictSets> sopnetConflictSets = pipeline::Value<ConflictSets>();
	
	LOG_USER(out) << "Read " << stack->size() << " images" << std::endl;
	
	// Extract Slices as in the original SOPNET pipeline.
	foreach (boost::shared_ptr<Image> image, *stack)
	{
		boost::shared_ptr<SliceExtractor<unsigned char> > extractor =
			boost::make_shared<SliceExtractor<unsigned char> >(i++);
		pipeline::Value<Slices> slices;
		pipeline::Value<ConflictSets> conflictSets;
		extractor->setInput("membrane", image);
		slices = extractor->getOutput("slices");
		conflictSets = extractor->getOutput("conflict sets");
		
		sopnetSlices->addAll(*slices);
		sopnetConflictSets->addAll(*conflictSets);
		LOG_USER(out) << "Read " << slices->size() << " slices" << std::endl;
	}
	
	LOG_USER(out) << "Read " << sopnetSlices->size() << " slices altogether" << std::endl;
	
	// Blockwise variables
	boost::shared_ptr<SliceGuarantor> sliceGuarantor = boost::make_shared<SliceGuarantor>();
	boost::shared_ptr<BlockManager> blockManager = boost::make_shared<LocalBlockManager>(stackSize,
																					blockSize);
	boost::shared_ptr<StackStore> stackStore = boost::make_shared<LocalStackStore>(membranePath);
	boost::shared_ptr<Box<> > stackBox = boost::make_shared<Box<> >(util::point3<unsigned int>(0,0,0),
															   stackSize);
	boost::shared_ptr<Blocks> blocks = blockManager->blocksInBox(stackBox);
	boost::shared_ptr<SliceStore> sliceStore = boost::make_shared<LocalSliceStore>();
	
	boost::shared_ptr<SliceReader> sliceReader = boost::make_shared<SliceReader>();
	
	pipeline::Value<Slices> blockwiseSlices;
	pipeline::Value<ConflictSets> blockwiseConflictSets;
	
	sliceGuarantor->setInput("slice store", sliceStore);
	sliceGuarantor->setInput("stack store", stackStore);
	
	
	// Now, do it blockwise
	if (optionCoreTestSequential)
	{		
		foreach (const boost::shared_ptr<Block> block, *blocks)
		{
			pipeline::Value<Blocks> needBlocks;
			boost::shared_ptr<Blocks> singleBlock = blockManager->blocksInBox(block);
			sliceGuarantor->setInput("blocks", singleBlock);
			
			LOG_USER(out) << "Extracting slices from block " << *block << endl;
			
			needBlocks = sliceGuarantor->guaranteeSlices();
			
			if (!needBlocks->empty())
			{
				LOG_USER(out) << "SliceGuarantor needs images for block " <<
					*block << endl;
				return false;
			}
		}
	}
	else
	{
		pipeline::Value<Blocks> needBlocks;
		
		LOG_USER(out) << "Extracting slices from entire stack" << endl;
		
		sliceGuarantor->setInput("blocks", blocks);
		
		needBlocks = sliceGuarantor->guaranteeSlices();
		
		if (!needBlocks->empty())
		{
			LOG_USER(out) << "SliceGuarantor needs images for " <<
				needBlocks->length() << " blocks:" << endl;
			foreach (boost::shared_ptr<Block> block, *needBlocks)
			{
				LOG_USER(out) << "\t" << *block << endl;
			}
			return false;
		}
	}
	
	sliceReader->setInput("blocks", blocks);
	sliceReader->setInput("store", sliceStore);
	
	blockwiseSlices = sliceReader->getOutput("slices");
	blockwiseConflictSets = sliceReader->getOutput("conflict sets");
	
	LOG_USER(out) << "Read " << blockwiseSlices->size() << " slices block-wise" << endl;
	
	// Compare outputs
	bool ok = true;
	
	// Slices in blockwiseSlices that are not in sopnetSlices
	boost::shared_ptr<Slices> bsSlicesSetDiff = boost::make_shared<Slices>();
	// vice-versa
	boost::shared_ptr<Slices> sbSlicesSetDiff = boost::make_shared<Slices>();
	boost::shared_ptr<ConflictSets> bsConflictSetDiff = boost::make_shared<ConflictSets>();
	boost::shared_ptr<ConflictSets> sbConflictSetDiff = boost::make_shared<ConflictSets>();
	// Here, we will store blockwise conflict sets in which the slice ids are mapped to the
	// equivalent slide id in the sopnet slices. This allows us to compare conflict sets by their
	// == operator.
	boost::shared_ptr<ConflictSets> blockwiseConflictSetsSopnetIds;
	// Maps blockwise ids to sopnet ids.
	std::map<unsigned int, unsigned int> blockwiseSopnetIDMap;
	
	foreach (boost::shared_ptr<Slice> blockwiseSlice, *blockwiseSlices)
	{
		int id = sliceContains(sopnetSlices, blockwiseSlice);
		
		if (id < 0)
		{
			bsSlicesSetDiff->add(blockwiseSlice);
			ok = false;
		}
		else
		{
			unsigned int uid = (unsigned int)id;
			blockwiseSopnetIDMap[blockwiseSlice->getId()] = uid;
		}
	}
	
	foreach (boost::shared_ptr<Slice> sopnetSlice, *sopnetSlices)
	{
		int id = sliceContains(blockwiseSlices, sopnetSlice);
		
		if (id < 0)
		{
			sbSlicesSetDiff->add(sopnetSlice);
			ok = false;
		}
	}
	
	if (!ok)
	{
		LOG_USER(out) << bsSlicesSetDiff->size() <<
			" slices were found in the blockwise output but not sopnet: " << endl;
		foreach (boost::shared_ptr<Slice> slice, *bsSlicesSetDiff)
		{
			LOG_USER(out) << slice->getId() << ", " << slice->hashValue() << endl;
		}
		
		LOG_USER(out) << bsSlicesSetDiff->size() <<
			" slices were found in the sopnet output but not blockwise: " << endl;
		foreach (boost::shared_ptr<Slice> slice, *sbSlicesSetDiff)
		{
			LOG_USER(out) << slice->getId() << ", " << slice->hashValue() << endl;
		}
	}
	
	if (ok)
	{
		blockwiseConflictSetsSopnetIds = 
			mapConflictSets(blockwiseConflictSets, blockwiseSopnetIDMap);
		LOG_USER(out) << "Blockwise conflict sets mapped to sopnet ids" << std::endl;
		
		foreach (ConflictSet blockwiseConflict, *blockwiseConflictSetsSopnetIds)
		{
			if (!conflictSetContains(sopnetConflictSets, blockwiseConflict))
			{
				bsConflictSetDiff->add(blockwiseConflict);
				ok = false;
			}
		}
		
		foreach (ConflictSet sopnetConflict, *sopnetConflictSets)
		{
			if (!conflictSetContains(blockwiseConflictSetsSopnetIds, sopnetConflict))
			{
				
				sbConflictSetDiff->add(sopnetConflict);
				ok = false;
			}
		}
		
		if (!ok)
		{
			LOG_USER(out) << bsConflictSetDiff->size() <<
				" ConflictSets were found in the blockwise output but not sopnet:" << endl;
			foreach (ConflictSet conflictSet, *bsConflictSetDiff)
			{
				LOG_USER(out) << conflictSet << endl;
			}
			
			LOG_USER(out) << sbConflictSetDiff->size() <<
				" ConflictSets were found in the sopnet output but not blockwise:" << endl;
			foreach (ConflictSet conflictSet, *sbConflictSetDiff)
			{
				LOG_USER(out) << conflictSet << endl;
			}
		}
	}
	
	writeSlices(sopnetSlices, sopnetOutputPath);
	writeSlices(blockwiseSlices, blockwiseOutputPath);
	
	if (optionCoreTestWriteDebugFiles && blockwiseConflictSetsSopnetIds)
	{
		writeConflictSets(sopnetConflictSets, sopnetOutputPath);
		writeConflictSets(blockwiseConflictSetsSopnetIds, blockwiseOutputPath);
	}
	
	if (!ok)
	{
		LOG_USER(out) << "Slice test failed" << endl;
	}
	else
	{
		LOG_USER(out) << "Slice test passed" << endl;
	}
	
	return ok;
}

bool guaranteeSegments(const boost::shared_ptr<Blocks> blocks,
	const boost::shared_ptr<SliceStore> sliceStore,
	const boost::shared_ptr<SegmentStore> segmentStore,
	const boost::shared_ptr<StackStore> stackStore)
{
	boost::shared_ptr<SliceGuarantor> sliceGuarantor = boost::make_shared<SliceGuarantor>();
	boost::shared_ptr<SegmentGuarantor> segmentGuarantor = boost::make_shared<SegmentGuarantor>();
	boost::shared_ptr<BlockManager> blockManager = blocks->getManager();
	
	sliceGuarantor->setInput("slice store", sliceStore);
	sliceGuarantor->setInput("stack store", stackStore);
	segmentGuarantor->setInput("segment store", segmentStore);
	segmentGuarantor->setInput("slice store", sliceStore);
	
	if (optionCoreTestSequential)
	{
		foreach (const boost::shared_ptr<Block> block, *blocks)
		{
			int count = 0;
			pipeline::Value<Blocks> needBlocks;
			boost::shared_ptr<Blocks> singleBlock = blockManager->blocksInBox(block);
			segmentGuarantor->setInput("blocks", singleBlock);
			
			do
			{
				needBlocks = segmentGuarantor->guaranteeSegments();
				if (!needBlocks->empty())
				{
					sliceGuarantor->setInput("blocks", needBlocks);
					sliceGuarantor->guaranteeSlices();
				}
				++count;
				
				// If we have run more times than there are blocks in the block manager, then there
				// is definitely something wrong.
				if (count > (int)(blocks->length() + 2))
				{
					LOG_USER(out) << "SegmentGuarantor test: too many iterations" << endl;
					return false;
				}
			} while(!needBlocks->empty());
		}
	}
	else
	{
		pipeline::Value<Blocks> needBlocks;
		
		LOG_USER(out) << "Extracting slices from entire stack" << endl;
		
		sliceGuarantor->setInput("blocks", blocks);
		segmentGuarantor->setInput("blocks", blocks);
		
		sliceGuarantor->guaranteeSlices();
		needBlocks = segmentGuarantor->guaranteeSegments();
		
		if (!needBlocks->empty())
		{
			LOG_USER(out) << "SegmentGuarantor says it needs slices for " <<
				needBlocks->length() << ", but we extracted slices for all of them" << endl;
			foreach (boost::shared_ptr<Block> block, *needBlocks)
			{
				LOG_USER(out) << "\t" << *block << endl;
			}
			return false;
		}
	}
	
	return true;
}

bool testSegments(util::point3<unsigned int> stackSize, util::point3<unsigned int> blockSize)
{
	// SOPNET variables
	int i = 0;
	bool segExtraction = false, ok = true;
	std::string membranePath = optionCoreTestMembranesPath.as<std::string>();
	
	boost::shared_ptr<ImageStackDirectoryReader> directoryStackReader =
			boost::make_shared<ImageStackDirectoryReader>(membranePath);
	pipeline::Value<ImageStack> stack = directoryStackReader->getOutput();
	
	pipeline::Value<Segments> sopnetSegments = pipeline::Value<Segments>();
	pipeline::Value<Slices> prevSlices, nextSlices;
	
	// Extract Segments as in the original SOPNET pipeline.
	foreach (boost::shared_ptr<Image> image, *stack)
	{
		boost::shared_ptr<SliceExtractor<unsigned char> > extractor =
			boost::make_shared<SliceExtractor<unsigned char> >(i++);
		extractor->setInput("membrane", image);
		nextSlices = extractor->getOutput("slices");
		
		if (segExtraction)
		{
			// bfe: Bool Force Explanation
			bool bfe = optionCoreTestForceExplanation;
			pipeline::Value<bool> forceExplanation =
				pipeline::Value<bool>(bfe);
			boost::shared_ptr<SegmentExtractor> segmentExtractor =
				boost::make_shared<SegmentExtractor>();
			pipeline::Value<Segments> segments;
			
			segmentExtractor->setInput("previous slices", prevSlices);
			segmentExtractor->setInput("next slices", nextSlices);
			segmentExtractor->setInput("force explanation", forceExplanation);
			
			segments = segmentExtractor->getOutput("segments");
			sopnetSegments->addAll(segments);
		}
		
		segExtraction = true;
		prevSlices = nextSlices;
	}
	
	// Blockwise variables
	boost::shared_ptr<BlockManager> blockManager = boost::make_shared<LocalBlockManager>(stackSize,
																					blockSize);
	boost::shared_ptr<StackStore> stackStore = boost::make_shared<LocalStackStore>(membranePath);
	boost::shared_ptr<Box<> > stackBox = boost::make_shared<Box<> >(util::point3<unsigned int>(0,0,0),
															   stackSize);
	boost::shared_ptr<Blocks> blocks = blockManager->blocksInBox(stackBox);
	boost::shared_ptr<SliceStore> sliceStore = boost::make_shared<LocalSliceStore>();
	boost::shared_ptr<SegmentStore> segmentStore = boost::make_shared<LocalSegmentStore>();
	
	boost::shared_ptr<SegmentReader> segmentReader = boost::make_shared<SegmentReader>();
	
	pipeline::Value<Segments> blockwiseSegments;
	
	// Now, do it blockwise
	if (!guaranteeSegments(blocks, sliceStore, segmentStore, stackStore))
	{
		return false;
	}
	
	
	segmentReader->setInput("blocks", blocks);
	segmentReader->setInput("store", segmentStore);
	
	blockwiseSegments = segmentReader->getOutput("segments");
	
	// Now, check for differences
	
	// Segments that appear in the blockwise segments, but not the sopnet segments
	boost::shared_ptr<Segments> bsSegmentSetDiff = boost::make_shared<Segments>();
	// vice-versa
	boost::shared_ptr<Segments> sbSegmentSetDiff = boost::make_shared<Segments>();

	foreach (boost::shared_ptr<Segment> blockWiseSegment, blockwiseSegments->getSegments())
	{
		if (!segmentsContains(sopnetSegments, blockWiseSegment))
		{
			bsSegmentSetDiff->add(blockWiseSegment);
			ok = false;
		}
	}
	
	foreach (boost::shared_ptr<Segment> sopnetSegment, sopnetSegments->getSegments())
	{
		if (!segmentsContains(blockwiseSegments, sopnetSegment))
		{
			sbSegmentSetDiff->add(sopnetSegment);
			ok = false;
		}
	}
	
	if (optionCoreTestWriteDebugFiles)
	{
		writeSegments(sopnetSegments, sopnetOutputPath);
		writeSegments(blockwiseSegments, blockwiseOutputPath);
	}
	
	if (!ok)
	{
		LOG_USER(out) << bsSegmentSetDiff->size() <<
			" segments were found in the blockwise output but not sopnet: " << endl;
		if (bsSegmentSetDiff->size() > 0)
		{
			LOG_USER(out) << "\t" << bsSegmentSetDiff->getEnds().size() <<
				" Ends" << endl;
			LOG_USER(out) << "\t" << bsSegmentSetDiff->getContinuations().size() <<
				" Continuations" << endl;
			LOG_USER(out) << "\t" << bsSegmentSetDiff->getBranches().size() <<
				" Branches" << endl;
			
		}
		foreach (boost::shared_ptr<Segment> segment, bsSegmentSetDiff->getSegments())
		{
			LOG_USER(out) << Segment::typeString(segment->getType()) << " ";
			foreach (boost::shared_ptr<Slice> slice, segment->getSlices())
			{
				LOG_USER(out) << slice->getId() << " ";
			}
			LOG_USER(out) << endl;
		}
		
		LOG_USER(out) << sbSegmentSetDiff->size() <<
			" segments were found in the sopnet output but not blockwise: " << endl;
		if (sbSegmentSetDiff->size() > 0)
		{
			LOG_USER(out) << "\t" << sbSegmentSetDiff->getEnds().size() <<
				" Ends" << endl;
			LOG_USER(out) << "\t" << sbSegmentSetDiff->getContinuations().size() <<
				" Continuations" << endl;
			LOG_USER(out) << "\t" << sbSegmentSetDiff->getBranches().size() <<
				" Branches" << endl;
			
		}
		foreach (boost::shared_ptr<Segment> segment, sbSegmentSetDiff->getSegments())
		{
			LOG_USER(out) << Segment::typeString(segment->getType()) << " ";
			foreach (boost::shared_ptr<Slice> slice, segment->getSlices())
			{
				LOG_USER(out) << slice->getId() << " ";
			}
			LOG_USER(out) << endl;
		}
	}

	if (ok)
	{
		LOG_USER(out) << "Segment test passed" << endl;
	}
	else
	{
		LOG_USER(out) << "Segment test failed" << endl;
	}
	
	return ok;
}

bool coreSolver(
	const boost::shared_ptr<SegmentationCostFunctionParameters> segmentationCostParameters,
	const boost::shared_ptr<PriorCostFunctionParameters>& priorCostFunctionParameters,
	const boost::shared_ptr<SegmentTrees>& neuronsOut,
	util::point3<unsigned int> stackSize,
	util::point3<unsigned int> blockSize)
{
	std::string membranePath = optionCoreTestMembranesPath.as<std::string>();
	std::string rawPath = optionCoreTestRawImagesPath.as<std::string>();
	
	boost::shared_ptr<BlockManager> blockManager =
		boost::make_shared<LocalBlockManager>(stackSize, blockSize);
	boost::shared_ptr<Box<> > box =
		boost::make_shared<Box<> >(util::point3<unsigned int>(0u, 0u, 0u), stackSize);
	pipeline::Value<unsigned int> maxSize(1024 * 1024 * 64);
	boost::shared_ptr<Blocks> blocks = blockManager->blocksInBox(box);
	
	// Block pipeline variables
	boost::shared_ptr<StackStore> membraneStackStore = boost::make_shared<LocalStackStore>(membranePath);
	boost::shared_ptr<StackStore> rawStackStore = boost::make_shared<LocalStackStore>(rawPath);
	boost::shared_ptr<LocalSliceStore> localSliceStore =
		boost::shared_ptr<LocalSliceStore>(new LocalSliceStore());
	boost::shared_ptr<SliceStore> sliceStore = localSliceStore;
	boost::shared_ptr<SegmentStore> segmentStore =
		boost::shared_ptr<SegmentStore>(new LocalSegmentStore());
	boost::shared_ptr<SegmentReader> segmentReader = boost::make_shared<SegmentReader>();
	boost::shared_ptr<SliceReader> sliceReader= boost::make_shared<SliceReader>();
	boost::shared_ptr<CoreSolver> coreSolver = boost::make_shared<CoreSolver>();
	bool bfe = optionCoreTestForceExplanation;
	pipeline::Value<bool> forceExplanation = pipeline::Value<bool>(bfe);
	
	// Result Values
	//pipeline::Value<SliceStoreResult> sliceResult;
	pipeline::Value<SegmentTrees> neurons;
	pipeline::Value<Segments> segments;
	pipeline::Value<Slices> slices;
	
	// guarantee segments
	if (!guaranteeSegments(blocks, sliceStore, segmentStore, membraneStackStore))
	{
		return false;
	}

	LOG_USER(out) << "Setting up block solver" << std::endl;
	coreSolver->setInput("prior cost parameters",
							priorCostFunctionParameters);
	coreSolver->setInput("segmentation cost parameters", segmentationCostParameters);
	coreSolver->setInput("blocks", blocks);
	coreSolver->setInput("segment store", segmentStore);
	coreSolver->setInput("slice store", sliceStore);
	coreSolver->setInput("raw image store", rawStackStore);
	coreSolver->setInput("membrane image store", membraneStackStore);
	coreSolver->setInput("force explanation", forceExplanation);
	
	LOG_USER(out) << "Inputs are set" << endl;
	
	neurons = coreSolver->getOutput("neurons");
	segments = coreSolver->getOutput("segments");
	
	LOG_USER(out) << "Solved " << neurons->size() << " neurons" << std::endl;
	
	neuronsOut->addAll(neurons);
	
	return true;
}

void sopnetSolver(
	const boost::shared_ptr<SegmentationCostFunctionParameters> segmentationCostParameters,
	const boost::shared_ptr<PriorCostFunctionParameters>& priorCostFunctionParameters,
	const boost::shared_ptr<SegmentTrees>& neuronsOut)
{
	std::string membranePath = optionCoreTestMembranesPath.as<std::string>();
	std::string rawPath = optionCoreTestRawImagesPath.as<std::string>();
	boost::shared_ptr<ImageStackDirectoryReader> membraneReader =
		boost::make_shared<ImageStackDirectoryReader>(membranePath);
	boost::shared_ptr<ImageStackDirectoryReader> rawReader =
		boost::make_shared<ImageStackDirectoryReader>(rawPath);
	boost::shared_ptr<Sopnet> sopnet = boost::make_shared<Sopnet>("woo hoo");
	boost::shared_ptr<NeuronExtractor> neuronExtractor = boost::make_shared<NeuronExtractor>();
	
	bool bfe = optionCoreTestForceExplanation;
	pipeline::Value<SegmentTrees> neurons;
	pipeline::Value<Segments> segments;
	pipeline::Value<bool> forceExplanation = pipeline::Value<bool>(bfe);
	
	sopnet->setInput("raw sections", rawReader->getOutput());
	sopnet->setInput("membranes", membraneReader->getOutput());
	sopnet->setInput("neuron slices", membraneReader->getOutput());
	sopnet->setInput("segmentation cost parameters", segmentationCostParameters);
	sopnet->setInput("prior cost parameters", priorCostFunctionParameters);
	sopnet->setInput("force explanation", forceExplanation);
	neuronExtractor->setInput("segments", sopnet->getOutput("solution"));
	
	neurons = neuronExtractor->getOutput();
	segments = sopnet->getOutput("segments");
	
	LOG_USER(out) << "Solved " << neurons->size() << " neurons" << std::endl;
	
	neuronsOut->addAll(neurons);
}

bool testSolutions(util::point3<unsigned int> stackSize, util::point3<unsigned int> blockSize)
{
	bool ok;
	
	boost::shared_ptr<SegmentTrees> sopnetNeurons = boost::make_shared<SegmentTrees>();
	boost::shared_ptr<SegmentTrees> blockwiseNeurons = boost::make_shared<SegmentTrees>();
	boost::shared_ptr<SegmentationCostFunctionParameters> segmentationCostParameters = 
		boost::make_shared<SegmentationCostFunctionParameters>();
	boost::shared_ptr<PriorCostFunctionParameters> priorCostFunctionParameters = 
		boost::make_shared<PriorCostFunctionParameters>();
	
	segmentationCostParameters->weightPotts = 0;
	segmentationCostParameters->weight = 0;
	segmentationCostParameters->priorForeground = 0.2;
	
	priorCostFunctionParameters->priorContinuation = -50;
	priorCostFunctionParameters->priorBranch = -100;
	
	sopnetSolver(segmentationCostParameters, priorCostFunctionParameters, sopnetNeurons);
	ok = coreSolver(segmentationCostParameters, priorCostFunctionParameters, blockwiseNeurons,
			   stackSize, blockSize);
	
	return ok;
}


util::point3<unsigned int> parseBlockSize(const util::point3<unsigned int> stackSize)
{
	std::string blockSizeFraction = optionCoreTestBlockSizeFraction.as<std::string>();
    std::string item;
	util::point3<unsigned int> blockSize;
	int num, denom;

	std::size_t slashPos = blockSizeFraction.find("/");
	
	if (slashPos == std::string::npos)
	{
		LOG_USER(out) << "Got no fraction in block size parameter, setting to stack size" <<
			endl;
		blockSize = stackSize;
	}
	else
	{
		std::string numStr = blockSizeFraction.substr(0, slashPos);
		std::string denomStr = blockSizeFraction.substr(slashPos + 1, std::string::npos);
		
		LOG_USER(out) << "Got numerator " << numStr << ", denominator " << denomStr << endl;
		num = boost::lexical_cast<int>(numStr);
		denom = boost::lexical_cast<int>(denomStr);
		
		blockSize = util::point3<unsigned int>(fractionCeiling(stackSize.x, num, denom),
										fractionCeiling(stackSize.y, num, denom),
										stackSize.z);
	}

	LOG_USER(out) << "Stack size: " << stackSize << ", block size: " << blockSize << endl;
	
	return blockSize;
}

void mkdir(const std::string& path)
{
	boost::filesystem::path dir(path);
	boost::filesystem::create_directory(dir);
}

int main(int optionc, char** optionv)
{
	util::ProgramOptions::init(optionc, optionv);
	LogManager::init();
	
	try
	{
		std::string membranePath = optionCoreTestMembranesPath.as<std::string>();
		pipeline::Value<ImageStack> testStack;
		unsigned int nx, ny, nz;
		util::point3<unsigned int> stackSize, blockSize;
		
		boost::shared_ptr<ImageStackDirectoryReader> directoryStackReader =
			boost::make_shared<ImageStackDirectoryReader>(membranePath);
		
		testStack = directoryStackReader->getOutput();
		nx = testStack->width();
		ny = testStack->height();
		nz = testStack->size();
		
		stackSize = point3<unsigned int>(nx, ny, nz);
		
		blockSize = parseBlockSize(stackSize);
		
		if (optionCoreTestWriteSliceImages || optionCoreTestWriteDebugFiles)
		{
			blockwiseOutputPath = blockwiseOutputPath + "_" +
				boost::lexical_cast<std::string>(stackSize.x) + "x" + 
				boost::lexical_cast<std::string>(stackSize.y) + "_" + 
				boost::lexical_cast<std::string>(blockSize.x) + "x" +
				boost::lexical_cast<std::string>(blockSize.y);
			mkdir(sopnetOutputPath);
			mkdir(blockwiseOutputPath);
		}
		
		testStack->clear();
		
		if (!testSlices(stackSize, blockSize))
		{
			
			return -1;
		}
		
		if (!testSegments(stackSize, blockSize))
		{
			return -2;
		}
		
// 		if (!testSolutions(stackSize, blockSize))
// 		{
// 			return -3;
// 		}
		
		return 0;
	}
	catch (Exception& e)
	{
		handleException(e);
	}
}
