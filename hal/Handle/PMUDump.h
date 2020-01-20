#pragma once

#include "hal/Handle/DumpMixin.h"
#include "hal/Handle/PMU.h"

namespace HMF {
namespace Handle {

struct PMUDump :
	public PMU,
	public DumpMixin
{
	PMUDump(DumpMixin::ref_t dumper) :
		PMU(), DumpMixin(dumper)
	{}

	explicit PMUDump(DumpMixin::ref_t dumper, halco::hicann::v2::IPv4 const &) :
		PMU(), DumpMixin(dumper)
	{}

	PMUDump(DumpMixin::ref_t dumper, halco::hicann::v2::PMU const &, halco::hicann::v2::IPv4 const &) :
		PMU(), DumpMixin(dumper)
	{}
};

} // namespace Handle
} // namespace HMF
