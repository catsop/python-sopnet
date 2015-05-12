#ifndef TED_EVAULATION_EXTRACT_GROUND_TRUTH_LABELS_H__
#define TED_EVAULATION_EXTRACT_GROUND_TRUTH_LABELS_H__

#include <pipeline/SimpleProcessNode.h>
#include <imageprocessing/ImageStack.h>

class ExtractLabels : public pipeline::SimpleProcessNode<> {

public:

	ExtractLabels();

private:

	void updateOutputs();

	pipeline::Input<ImageStack>  _stack;
	pipeline::Output<ImageStack> _labelStack;
};

#endif // TED_EVAULATION_EXTRACT_GROUND_TRUTH_LABELS_H__

