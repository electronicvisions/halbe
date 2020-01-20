#pragma once

#include <cstdlib>
#include <bitset>

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/bitset.hpp>

#include "pywrap/compat/macros.hpp"

namespace HMF {
namespace HICANN {

struct Merger
{
public:
	// In the corresponding figure of the HICANN-doc all the register/merger
	// numbers are swapped 0<->7. !BUT! halco::common::left/halco::common::right are still correct-around
	static const size_t enable_bit = 1, select_bit = 0;
	static const uint8_t
		       LEFT_ONLY  = 0x0 << enable_bit | 0x1 << select_bit,
			   RIGHT_ONLY = 0x0 << enable_bit | 0x0 << select_bit,
			   MERGE      = 0x1 << enable_bit | 0x0 << select_bit
	;

	std::bitset<2> config; //!< configures merging or exclusive behaviour
	bool slow;             //!< has to be enabled for DNC Merger connecting to SPL1 HICANN::Repeater

	PYPP_CONSTEXPR Merger(size_t config = MERGE, bool slow = false) :
		config(config), slow(slow) {}

	bool operator ==(Merger const& b) const;
	bool operator !=(Merger const& b) const;

private:
	friend std::ostream& operator<<(std::ostream& os, const Merger & m);

	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const);
};



template<typename Archiver>
void Merger::serialize(Archiver& ar, unsigned int const)
{
	ar & boost::serialization::make_nvp("config", config)
	   & boost::serialization::make_nvp("slow", slow);
}

} // HICANN
} // HMF
