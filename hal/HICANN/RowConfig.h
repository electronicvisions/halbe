#pragma once

#include <boost/serialization/nvp.hpp>
#include <boost/serialization/bitset.hpp>
#include <boost/serialization/array.h>

#include "pywrap/compat/macros.hpp"
#include "pywrap/compat/array.hpp"
#include "pywrap/compat/rant.hpp"
#include "halco/common/relations.h"
#include "hal/HICANN/DriverDecoder.h"

namespace HMF {
namespace HICANN {

class GmaxDiv :
	public halco::common::detail::RantWrapper<GmaxDiv, int, 30, 0>
{
public:
	explicit PYPP_CONSTEXPR GmaxDiv(uint8_t val = 0) : rant_t(val) {}
};

/**
 * Configuration data for one synapse line
 */
struct RowConfig
{
public:
	static const size_t	num_decoder = 2;   //!< number of of decoder per row
	static const size_t	num_syn_ins = 2;   //!< number of synaptic input circuits per denmem

	RowConfig();
	PYPP_DEFAULT(PYPP_CONSTEXPR RowConfig(RowConfig const&));
	PYPP_DEFAULT(RowConfig& operator=(RowConfig const&));

	/// Connects all synapses of this row to the synaptic input
	/// of the neuron. The synaptic input is specified by
	/// left = Esyni and right = Esynx
	/// both can be connected
	bool get_syn_in(halco::common::Side const& s) const;
	void set_syn_in(halco::common::Side const& s, bool b);

	DriverDecoder const& get_decoder(halco::common::SideVertical const& s) const;
	void set_decoder(halco::common::SideVertical const& s, DriverDecoder const& d);

	uint8_t get_gmax() const;
	void set_gmax(uint8_t v);

	uint8_t get_gmax_div(halco::common::Side const& s) const;
	void set_gmax_div(halco::common::Side const& s, uint8_t v);

	/// There are two parallel circuits for the divisor in the synapse
	/// driver (left, right), the sum of both divisor values gives the
	/// actual divisor value applied to V_gmax
	void set_gmax_div(GmaxDiv const& v);

	bool operator== (RowConfig const& b) const;
	bool operator!= (RowConfig const& b) const;

private:
	/// choose synaptic input of denmem circuits this line is connected to.
	/// corresponds to `senx` and `seni` in hicann-doc, with syn_in[0] == senx
	std::bitset<num_syn_ins> syn_in;

	/// defines, for even and odd denmem addresses to which 2 MSBits of an spl1 address this row listens to.
	/// values: 0-3. decoder[0] is for left (even) synapses
	std::array<DriverDecoder, num_decoder> decoder;

	/// select V_gmax from shared FGs. Values: 0-3
	rant::integral_range<uint8_t, 3> selgmax;

	/// g_max divisor for connection to input of neurons. Values: 0-15. Reasonable values: 1-15
	/// g_max divisor can be set separately for syn_in[0] and syn_in[1]. Respectively gmax_div[0] and [1]
	std::array<rant::integral_range<uint8_t, 15>, num_syn_ins> gmax_div;

	friend class boost::serialization::access;
	template<typename Archiver>
	void serialize(Archiver& ar, unsigned int const);

	friend std::ostream& operator<<(std::ostream& os, const RowConfig & m);
};



template<typename Archiver>
void RowConfig::serialize(Archiver& ar, unsigned int const)
{
	using boost::serialization::make_nvp;
	ar & make_nvp("syn_in", syn_in)
	   & make_nvp("selgmax", selgmax)
	   & make_nvp("gmax_div", gmax_div)
	   & make_nvp("decoder", decoder);
}

} // HICANN
} // HMF

namespace std {

HALCO_GEOMETRY_HASH_CLASS(HMF::HICANN::GmaxDiv)

}
