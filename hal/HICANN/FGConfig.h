#pragma once

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/bitset.hpp>

namespace HMF {
namespace HICANN {

struct FGConfig
{
public:
	FGConfig();

	// FIXME: ugly as hell
	// operation registers

	/// Maximum number of programming cycles.
	std::bitset<8> maxcycle;
	/// Time waited until value to be read is stable
	std::bitset<6> readtime;
	/// Number of cycles after which the write time is doubled
	std::bitset<6> acceleratorstep;
	/// Length of a write pulse
	std::bitset<6> voltagewritetime;
	/// Length of a write pulse
	std::bitset<6> currentwritetime;

	// bias registers
	/// Bias for upper floating gate programming voltage
	std::bitset<4> fg_bias;
	/// Bias for source followers. Maximum value if set to zero
	std::bitset<4> fg_biasn;
	/// Clock cycle multiplicator
	std::bitset<4> pulselength;
	/// shorts $V_m$ to ground
	std::bitset<1> groundvm;
	/// activate calibration mode
	std::bitset<1> calib;

	bool operator==(FGConfig const& rhs) const;
	bool operator!=(FGConfig const& rhs) const;

#ifndef PYPLUSPLUS
	std::bitset<14> getBias() const;
	std::bitset<32> getOp() const;
#endif

	/// Returns the maximum programming time for a single row
	/// of the floating gates in term of PLL cycles
	size_t getMaxCurrentProgrammingTime() const;
	size_t getMaxVoltageProgrammingTime() const;

private:
	size_t getMaxProgrammingTime(size_t pulselength) const;

	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const);

	friend std::ostream& operator<<(std::ostream& os, FGConfig const& fc);
};



template<typename Archiver>
void FGConfig::serialize(Archiver& ar, unsigned int const)
{
	using boost::serialization::make_nvp;
	ar & make_nvp("maxcycle", maxcycle)
	   & make_nvp("readtime", readtime)
	   & make_nvp("acceleratorstep", acceleratorstep)
	   & make_nvp("voltagewritetime", voltagewritetime)
	   & make_nvp("currentwritetime", currentwritetime)
	   & make_nvp("fg_bias", fg_bias)
	   & make_nvp("fg_biasn", fg_biasn)
	   & make_nvp("pulselength", pulselength)
	   & make_nvp("groundvm", groundvm)
	   & make_nvp("calib", calib);
}

} // HICANN
} // HMF
