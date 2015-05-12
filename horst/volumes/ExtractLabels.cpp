#include "ExtractLabels.h"
#include <vigra/multi_labeling.hxx>
#include <util/ProgramOptions.h>

util::ProgramOption optionKLargestComponents(
		util::_long_name        = "kLargestComponents",
		util::_description_text = "When extracting labels from a black/white image, only consider the k largest connected components.");

ExtractLabels::ExtractLabels() {

	registerInput(_stack, "stack");
	registerOutput(_labelStack, "label stack");
}

void
ExtractLabels::updateOutputs() {

	unsigned int width  = _stack->width();
	unsigned int height = _stack->height();
	unsigned int depth  = _stack->size();

	_labelStack = new ImageStack();
	for (unsigned int d = 0; d < depth; d++)
		_labelStack->add(boost::make_shared<Image>(width, height));

	vigra::MultiArray<3, float> gtVolume(vigra::Shape3(width, height, depth));
	unsigned int z = 0;
	foreach (boost::shared_ptr<Image> image, *_stack) {

		gtVolume.bind<2>(z) = *image;
		z++;
	}

	vigra::MultiArray<3, float> gtLabels(vigra::Shape3(width, height, depth));
	int numComponents = vigra::labelMultiArrayWithBackground(
			gtVolume,
			gtLabels);

	std::cout << "found " << numComponents << " connected components" << std::endl;

	if (optionKLargestComponents) {

		int k = optionKLargestComponents;

		std::vector<std::pair<size_t,int>> sizes(numComponents, std::make_pair(0, 0));
		for (auto& v : gtLabels)
			if (v != 0)
				sizes[v].first++;
		for (int i = 0; i < numComponents; i++)
			sizes[i].second = i;
		std::sort(sizes.rbegin(), sizes.rend());

		std::vector<bool> selected(numComponents, false);
		for (int i = 0; i < numComponents; i++) {

			if (i < k)
				std::cout << "accepting component " << sizes[i].second << std::endl;
			selected[sizes[i].second] = (i < k);
		}
		for (auto& v : gtLabels)
			if (!selected[v])
				v = 0;

		std::cout << "selected " << std::min(k, numComponents) << " components" << std::endl;
	}

	z = 0;
	foreach (boost::shared_ptr<Image> image, *_labelStack) {

		static_cast<vigra::MultiArray<2, float>& >(*image) = gtLabels.bind<2>(z);
		z++;
	}

	_labelStack->setResolution(_stack->getResolution());
	_labelStack->setOffset(_stack->getOffset());
}
