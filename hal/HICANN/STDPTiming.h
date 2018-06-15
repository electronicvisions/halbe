#pragma once

#include <bitset>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/bitset.hpp>

#include "pywrap/compat/macros.hpp"

namespace HMF {
namespace HICANN {

struct STDPTiming
{
	std::bitset<4> endel;  //enable delay in clocks
	std::bitset<4> slen;   //shared enable bits
	std::bitset<4> outdel; //output delay in clocks
	std::bitset<4> predel; //precharge delay in clocks
	std::bitset<2> wrdel;  //write delay in clocks

	PYPP_CONSTEXPR STDPTiming() :
		endel(0xf),
		slen(0xf),
		outdel(0xf),
		predel(0xf),
		wrdel(0x3)
	{}

	bool operator ==(STDPTiming const& b) const { return (b.endel == endel && b.slen == slen &&
		b.outdel == outdel && b.wrdel == wrdel && b.predel == predel); }
	bool operator!=(STDPTiming const& other) const { return !(*this == other); }

private:
	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const)
	{
		ar & boost::serialization::make_nvp("endel", endel);
		ar & boost::serialization::make_nvp("slen", slen);
		ar & boost::serialization::make_nvp("outdel", outdel);
		ar & boost::serialization::make_nvp("predel", predel);
		ar & boost::serialization::make_nvp("wrdel", wrdel);
	}

	friend std::ostream& operator<< (std::ostream& os, STDPTiming const& o);
};

} // end namespace HMF
} // end namespace HICANN
