#include "HICANNHw.h"

#include "hal/Handle/FPGAHw.h"

#include "reticle_control.h"

namespace HMF {
namespace Handle {

HICANNHw::HICANNHw(halco::hicann::v2::HICANNGlobal const& h,
                   const boost::shared_ptr<facets::ReticleControl>& rc, uint8_t jtag_addr, bool request_highspeed)
    : HICANN(h, request_highspeed), mReticleControl(rc), m_jtag_addr(jtag_addr) {}

HICANNHw::~HICANNHw()
{}

boost::shared_ptr<facets::ReticleControl> HICANNHw::get_reticle()
{
	if (auto rc = mReticleControl.lock())
		return rc;
	throw std::runtime_error("Lost reticle instance");
}

uint8_t HICANNHw::jtag_addr() const
{
	return m_jtag_addr;
}

}// namespace Handle
} // namespace HMF
