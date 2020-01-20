#include "hal/HICANN/L1Address.h"
#include "halco/hicann/v2/l1.h"

#include <bitter/integral.h>

namespace HMF {
namespace HICANN {

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
