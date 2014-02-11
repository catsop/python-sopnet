#ifndef SLICE_GUARANTOR_PARAMETERS_H__
#define SLICE_GUARANTOR_PARAMETERS_H__

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#include <pipeline/Data.h>
#include <sopnet/slices/Slices.h>

struct SliceGuarantorParameters : public pipeline::Data
{
    SliceGuarantorParameters() :
		guaranteeAllSlices(false),
		slicesToGuarantee(boost::make_shared<Slices>()) {}
    
    bool guaranteeAllSlices;
	boost::shared_ptr<Slices> slicesToGuarantee;
};


#endif //SLICE_GUARANTOR_PARAMETERS_H__