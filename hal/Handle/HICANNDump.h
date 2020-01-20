#pragma once

#include "hal/Handle/DumpMixin.h"
#include "hal/Handle/HICANN.h"

namespace HMF {
namespace Handle {

struct HICANNDump : public HICANN, DumpMixin
{
	HICANNDump(DumpMixin::ref_t dump, const halco::hicann::v2::HICANNGlobal & h) :
		HICANN(h),
		DumpMixin(dump)
	{}
};

} // namespace Handle
} // namespace HMF

