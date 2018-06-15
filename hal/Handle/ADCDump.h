#pragma once

#include "hal/Handle/ADC.h"
#include "hal/Handle/DumpMixin.h"

namespace HMF {
namespace Handle {

// TODO: if really private is meant... write it down!
struct ADCDump : public ADC, /*private?*/ DumpMixin
{
	/// Open connection to specific ADC (identified by its serial number == Coordinate)
	explicit ADCDump(DumpMixin::ref_t dumper, HMF::ADC::USBSerial const& serial) :
		ADC(serial),
		DumpMixin(dumper)
	{}

	/// Open connection to "first free" ADC
	explicit ADCDump(DumpMixin::ref_t dumper) :
		ADC(),
		DumpMixin(dumper)
	{}
};

} // namespace Handle
} // namespace HMF

