#pragma once

#include <bitset>
#include "halco/common/geometry.h"

namespace HMF {
namespace HICANN {

class SynapseDecoder :
	public halco::common::detail::RantWrapper<SynapseDecoder, uint8_t, 15, 0>
{
public:
	explicit PYPP_CONSTEXPR SynapseDecoder(uint8_t val = 0) : rant_t(val) {}
	static SynapseDecoder from_bitset(std::bitset<4> const& val) {
		return SynapseDecoder(val.to_ulong());
	}
};

} // end namespace HMF
} // namespace HICANN

namespace std {
HALCO_GEOMETRY_HASH_CLASS(HMF::HICANN::SynapseDecoder)
} // namespace std
