#include "hal/HICANN/RowConfig.h"

using namespace geometry;

namespace HMF {
namespace HICANN {

size_t const
	RowConfig::num_decoder,
	RowConfig::num_syn_ins;

RowConfig::RowConfig() :
	syn_in(0),
	decoder({{DriverDecoder(0), DriverDecoder(0)}}),
	selgmax(0),
	gmax_div({{1, 1}})
{}

bool RowConfig::get_syn_in(Side const& s) const
{
	return syn_in[s];
}

void RowConfig::set_syn_in(Side const& s, bool b)
{
	syn_in[s] = b;
}

DriverDecoder const& RowConfig::get_decoder(SideVertical const& s) const
{
	return decoder[s];
}

void RowConfig::set_decoder(SideVertical const& s, DriverDecoder const& d)
{
	decoder[s] = d;
}

uint8_t RowConfig::get_gmax() const
{
	return selgmax;
}

void RowConfig::set_gmax(uint8_t v)
{
	selgmax = v;
}

uint8_t RowConfig::get_gmax_div(Side const& s) const
{
	return gmax_div[s];
}

void RowConfig::set_gmax_div(Side const& s, uint8_t v)
{
	gmax_div[s] = v;
}

void RowConfig::set_gmax_div(GmaxDiv const& v)
{
	gmax_div[left] = std::min(v.value(), 15);
	gmax_div[right] = std::max(v.value() - 15, 0);
}

bool RowConfig::operator==(RowConfig const& b) const
{
	return syn_in == b.syn_in
		&& decoder == b.decoder
		&& selgmax == b.selgmax
		&& gmax_div == b.gmax_div;
}

bool RowConfig::operator!=(RowConfig const& b) const
{
	return !(*this == b);
}

std::ostream& operator<<(std::ostream& os, const HMF::HICANN::RowConfig & m)
{
	os << "RowConfig: Synaptic Input 0: " << m.syn_in[0] << ", Synaptic Input 1: " << m.syn_in[1] <<
	", Even Decoder: " << m.decoder[0] << ", Odd Decoder: " << m.decoder[1] << ", Gmax Line: " <<
	int(m.selgmax) << ", Gmax Divisor 0: " << int(m.gmax_div[0]) << ", Gmax Divisor 1: " << int(m.gmax_div[1]);
	return os;
}

} // HICANN
} // HMF
