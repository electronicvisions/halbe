#include "hal/Handle/FPGADump.h"

#include "hal/Coordinate/iter_all.h"

namespace HMF {
namespace Handle {

FPGADump::FPGADump(DumpMixin::ref_t dump, Coordinate::FPGAGlobal const c) :
	FPGA(c),
	Base(c),
	DumpMixin(dump)
{
	for (auto dnc : Coordinate::iter_all<HMF::Coordinate::DNCOnFPGA>() )
	{
		activate_dnc(dnc);
		for (auto hicann : Coordinate::iter_all<HMF::Coordinate::HICANNOnDNC>() )
			add_hicann(dnc, hicann);
	}
}


} // namespace Handle
} // namespace HMF

