#pragma once

#include <boost/make_shared.hpp>

#include "hal/Handle/DumpMixin.h"
#include "hal/Handle/FPGA.h"
#include "hal/Handle/HICANNDump.h"

namespace HMF {
namespace Handle {

struct FPGADump : public FPGAMixin<HICANNDump>, DumpMixin
{
	FPGADump(DumpMixin::ref_t dump, halco::hicann::v2::FPGAGlobal const c);

#ifndef PYPLUSPLUS
	hicann_handle_t create_hicann(halco::hicann::v2::HICANNGlobal const& h, bool /*request_highspeed*/) override
	{
		return boost::make_shared<HICANNDump>(dumper(), h);
	}
#endif
};

} // namespace Handle
} // namespace HMF

