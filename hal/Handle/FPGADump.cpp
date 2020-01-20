#include "hal/Handle/FPGADump.h"

#include "halco/common/iter_all.h"

namespace HMF {
namespace Handle {

FPGADump::FPGADump(DumpMixin::ref_t dump, halco::hicann::v2::FPGAGlobal const c) :
	FPGA(c),
	Base(c),
	DumpMixin(dump)
{
	for (auto dnc : halco::common::iter_all<halco::hicann::v2::DNCOnFPGA>() )
	{
		activate_dnc(dnc);
		for (auto hicann : halco::common::iter_all<halco::hicann::v2::HICANNOnDNC>() )
			add_hicann(dnc, hicann);
	}
}


} // namespace Handle
} // namespace HMF

