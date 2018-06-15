#pragma once

#include <boost/make_shared.hpp>

#include "hal/Handle/DumpMixin.h"
#include "hal/Handle/FPGA.h"
#include "hal/Handle/HICANNDump.h"

namespace HMF {
namespace Handle {

struct FPGADump : public FPGAMixin<HICANNDump>, DumpMixin
{
	FPGADump(DumpMixin::ref_t dump, Coordinate::FPGAGlobal const c);

#ifndef PYPLUSPLUS
	hicann_handle_t create_hicann(Coordinate::HICANNGlobal const& h) override
	{
		return boost::make_shared<HICANNDump>(dumper(), h);
	}
#endif
};

} // namespace Handle
} // namespace HMF

