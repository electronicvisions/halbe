#include "hal/HICANN/L1Address.h"
#include "halco/hicann/v2/l1.h"

#include <bitter/integral.h>
#include "bitter/util.h"

namespace HMF {
namespace HICANN {

L1Address::L1Address(DriverDecoder const& driver_decoder, SynapseDecoder const& synapse_decoder)
    : rant_t(bit::concat(
                 std::bitset<2>(driver_decoder.value()), std::bitset<4>(synapse_decoder.value()))
                 .to_ulong())
{}

SynapseDecoder L1Address::getSynapseDecoderMask() const
{
	return SynapseDecoder::from_bitset(bit::crop<4>(value()));
}

DriverDecoder L1Address::getDriverDecoderMask() const
{
	return DriverDecoder::from_bitset(bit::crop<2>(value(), 4));
}

} // end namespace HICANN
} // end namespace HMF
