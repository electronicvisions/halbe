#pragma once

#include "halco/hicann/v2/fwd.h"

#include "hal/HICANN/DriverDecoder.h"
#include "hal/HICANN/SynapseDecoder.h"

namespace HMF {
namespace HICANN {

class L1Address :
	public halco::common::detail::RantWrapper<L1Address, unsigned short, 63, 0>
{
public:
	PYPP_CONSTEXPR explicit L1Address(unsigned short val = 0) : rant_t(val) {}

	explicit L1Address(DriverDecoder const& driver_decoder, SynapseDecoder const& synapse_decoder);

	SynapseDecoder getSynapseDecoderMask() const;
	DriverDecoder getDriverDecoderMask() const;
};

} // end namespace HICANN
} // end namespace HMF

namespace std {
HALCO_GEOMETRY_HASH_CLASS(HMF::HICANN::L1Address)
} // namespace std
