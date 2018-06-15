#include "hal/HICANN/FGConfig.h"
#include <bitter/bitter.h>

namespace HMF {
namespace HICANN {

FGConfig::FGConfig() :
	maxcycle(255)
	, readtime(40)
	, acceleratorstep(9)
	, voltagewritetime(15)
	, currentwritetime(1)
	, fg_bias(8)
	, fg_biasn(5)
	, pulselength(9)
	, groundvm(0)
	, calib(0)
{}

bool FGConfig::operator==(FGConfig const& rhs) const
{
	return maxcycle == rhs.maxcycle
		&& readtime == rhs.readtime
		&& acceleratorstep == rhs.acceleratorstep
		&& voltagewritetime == rhs.voltagewritetime
		&& currentwritetime == rhs.currentwritetime
		&& fg_bias == rhs.fg_bias
		&& fg_biasn == rhs.fg_biasn
		&& pulselength == rhs.pulselength
		&& groundvm == rhs.groundvm
		&& calib == rhs.calib;
}

bool FGConfig::operator!=(FGConfig const& rhs) const
{
	return !(*this == rhs);
}

std::bitset<14> FGConfig::getBias() const
{
	return bit::concat(calib, groundvm, pulselength, fg_biasn, fg_bias);
}

std::bitset<32> FGConfig::getOp() const
{
	return bit::concat(currentwritetime, voltagewritetime,
		acceleratorstep, readtime, maxcycle);
}

std::ostream& operator<<(std::ostream& os, FGConfig const& fc)
{
	os << "FGConfig\n"
	   << "  maxcycle: " << fc.maxcycle.to_ulong() << "\n"
	   << "  readtime: " << fc.readtime.to_ulong() << "\n"
	   << "  acceleratorstep: " << fc.acceleratorstep.to_ulong() << "\n"
	   << "  voltagewritetime: " << fc.voltagewritetime.to_ulong() << "\n"
	   << "  currentwritetime: " << fc.currentwritetime.to_ulong() << "\n"
	   << "  fg_bias: " << fc.fg_bias.to_ulong() << "\n"
	   << "  fg_biasn: " << fc.fg_biasn.to_ulong() << "\n"
	   << "  pulselength: " << fc.pulselength.to_ulong() << "\n"
	   << "  groundvm: " << fc.groundvm.to_ulong() << "\n"
	   << "  calib: " << fc.calib.to_ulong() << "\n";
	return os;
}

size_t FGConfig::getMaxCurrentProgrammingTime() const
{
	return getMaxProgrammingTime(currentwritetime.to_ulong());
}

size_t FGConfig::getMaxVoltageProgrammingTime() const
{
	return getMaxProgrammingTime(voltagewritetime.to_ulong());
}

size_t FGConfig::getMaxProgrammingTime(size_t writetime) const
{
	// See hicann doc
	size_t cycles = 0;
	size_t rt = readtime.to_ulong() + 1;
	size_t pl = pulselength.to_ulong() + 1;
	size_t accelerator = acceleratorstep.to_ulong();;
	size_t max = maxcycle.to_ulong() + 1;
	size_t last_double = 0;
	for (size_t ii = 0; ii < max; ++ii)
	{
		// From HICANN doc:
		// The counter register that holds the write time is therefore 9 bits
		// wide although it can initially only be set to a 6 bit wide value.
		// The doubling only stops if the 8th bit of the write time register"
		// is set
		if (ii == (last_double + accelerator) and writetime < 128)
		{
			writetime *= 2;
			last_double = ii;
		}
		cycles += writetime * pl;
		cycles += rt * pl * 129;
	}
	// fg controller runs with slow clock, we want HICANN PLL as reference
	return cycles * 4;
}

} // HMF
} // HICANN
