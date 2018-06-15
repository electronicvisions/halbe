#include "hal/HICANN/STDPLUT.h"

#include "types-fpga.h" // struct fpga_pulse_packet_t

#include <bitter/integral.h>
#include <bitter/util.h>
#include <bitter/bitset.h>

using ::fpga_pulse_packet_t;

namespace HMF {
namespace HICANN {

std::bitset<32> STDPLUT::LUT::extract_low() const
{
	return bit::concat(
		bit::reverse(m_lut[0]),
		bit::reverse(m_lut[8]),
		bit::reverse(m_lut[4]),
		bit::reverse(m_lut[12]),
		bit::reverse(m_lut[2]),
		bit::reverse(m_lut[10]),
		bit::reverse(m_lut[6]),
		bit::reverse(m_lut[14]));
}

std::bitset<32> STDPLUT::LUT::extract_high() const
{
	return bit::concat(
		bit::reverse(m_lut[1]),
		bit::reverse(m_lut[9]),
		bit::reverse(m_lut[5]),
		bit::reverse(m_lut[13]),
		bit::reverse(m_lut[3]),
		bit::reverse(m_lut[11]),
		bit::reverse(m_lut[7]),
		bit::reverse(m_lut[15]));
}

void STDPLUT::LUT::import_low(std::bitset<32> data){
	m_lut[ 0] = bit::reverse(bit::crop<4>(data, 7*4));
	m_lut[ 8] = bit::reverse(bit::crop<4>(data, 6*4));
	m_lut[ 4] = bit::reverse(bit::crop<4>(data, 5*4));
	m_lut[12] = bit::reverse(bit::crop<4>(data, 4*4));
	m_lut[ 2] = bit::reverse(bit::crop<4>(data, 3*4));
	m_lut[10] = bit::reverse(bit::crop<4>(data, 2*4));
	m_lut[ 6] = bit::reverse(bit::crop<4>(data, 1*4));
	m_lut[14] = bit::reverse(bit::crop<4>(data, 0*4));
}

void STDPLUT::LUT::import_high(std::bitset<32> data){
	m_lut[ 0+1] = bit::reverse(bit::crop<4>(data, 7*4));
	m_lut[ 8+1] = bit::reverse(bit::crop<4>(data, 6*4));
	m_lut[ 4+1] = bit::reverse(bit::crop<4>(data, 5*4));
	m_lut[12+1] = bit::reverse(bit::crop<4>(data, 4*4));
	m_lut[ 2+1] = bit::reverse(bit::crop<4>(data, 3*4));
	m_lut[10+1] = bit::reverse(bit::crop<4>(data, 2*4));
	m_lut[ 6+1] = bit::reverse(bit::crop<4>(data, 1*4));
	m_lut[14+1] = bit::reverse(bit::crop<4>(data, 0*4));
}

void STDPLUT::set_defaults() {
	for (int i = 0; i < 16; i++) {
		causal[i] = std::min(i + 1, 15);
		acausal[i] = std::max(0, i - 1);
		combined[i] = i;
	}
}

std::ostream& operator<< (std::ostream& os, STDPLUT::LUT const& o)
{
	os << "LUT(";
	for (auto val : o.m_lut) {
		os << static_cast<int>(val.to_ulong()) << " ";
	}
	os << ")";
	return os;
}

std::ostream& operator<< (std::ostream& os, STDPLUT const& o)
{
	os << "STDPLUT(";
	os << "causal = " << o.causal << ", ";
	os << "acausal = " << o.acausal << ", ";
	os << "combined = " << o.combined << ")";
	return os;
}

} // end namespace HMF
} // end namespace HICANN
