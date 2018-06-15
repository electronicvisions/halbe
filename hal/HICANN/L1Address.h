#pragma once

#include "hal/Coordinate/geometry.h"

#include "hal/HICANN/DriverDecoder.h"
#include "hal/HICANN/SynapseDecoder.h"

namespace HMF {
namespace HICANN {

class L1Address :
	public Coordinate::detail::RantWrapper<L1Address, unsigned short, 63, 0>
{
public:
	PYPP_CONSTEXPR explicit L1Address(unsigned short val = 0) : rant_t(val) {}

	SynapseDecoder getSynapseDecoderMask() const;
	DriverDecoder getDriverDecoderMask() const;
};

} // end namespace HICANN
} // end namespace HMF

namespace std {
HALBE_GEOMETRY_HASH_CLASS(HMF::HICANN::L1Address)
} // namespace std
