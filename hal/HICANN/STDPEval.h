#pragma once

#include <bitset>

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/nvp.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/bitset.hpp>

namespace HMF {
namespace HICANN {

struct STDPEval
{
	static const size_t //enum to identify single factors
		ca = 0,
		ac = 1,
		aa = 2,
		cc = 3;

	//stdp_eval_bits are: MSB >> cc, aa, ac, ca << LSB
	std::bitset<4> causal;
	std::bitset<4> acausal;

	STDPEval():
		causal(0x1),
		acausal(0x2)
	{}

	STDPEval(std::bitset<4> const& c, std::bitset<4> const& a)
	    :	causal(c),
		acausal(a) {
	}

	bool operator ==(STDPEval const& b) const { return (b.causal == causal && b.acausal == acausal); }
	bool operator!=(STDPEval const& other) const { return !(*this == other); }

	std::bitset<8> extract_bits() const;
	void import_bits(std::bitset<8> data);

private:
	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const)
	{
		ar & boost::serialization::make_nvp("causal", causal);
		ar & boost::serialization::make_nvp("acausal", acausal);
	}

	friend std::ostream& operator<< (std::ostream& os, STDPEval const& o);
};

} // end namespace HMF
} // end namespace HICANN

