#pragma once

#include <bitset>
#include "hal/Coordinate/geometry.h"

namespace HMF {
namespace HICANN {

class DriverDecoder :
	public Coordinate::detail::RantWrapper<DriverDecoder, uint8_t, 3, 0>
{
public:
	explicit PYPP_CONSTEXPR DriverDecoder(uint8_t val = 0) : rant_t(val) {}
	static DriverDecoder from_bitset(std::bitset<2> const& val) {
		return DriverDecoder(val.to_ulong());
	}
};

} // end namespace HMF
} // namespace HICANN

namespace std {
HALBE_GEOMETRY_HASH_CLASS(HMF::HICANN::DriverDecoder)
} // namespace std
