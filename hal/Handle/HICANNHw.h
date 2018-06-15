#pragma once

#include <boost/weak_ptr.hpp>

#include "hal/Handle/HICANN.h"
#include "hal/Handle/FPGA.h"

namespace facets {
	struct Stage2Comm;
	struct ReticleControl;
}

namespace HMF {
namespace Handle {

struct FPGAHw;

/**
 * @brief (Ultra-Minimal) Handle class that encapsulates a single HICANN connection to hw system.
 *
 * @note This will(!) change a lot (issueCmd(), etc.)! Purpose: Adapt/think about parameters/comm hiding.
 * @note @CK destructor shouldn't be public, FIXME
 */
struct HICANNHw : public HICANN
{
	~HICANNHw();
	// Gets reticle (shouldn't be in the interface... FIXME: remove this from
	// public interface)
	PYPP_EXCLUDE(boost::shared_ptr<facets::ReticleControl> get_reticle();)
	uint8_t jtag_addr() const;
	bool isKintex() const;

	/// Construct a HICANN that is connected to FPGA f
	HICANNHw(Coordinate::HICANNGlobal const& h,
	         const boost::shared_ptr<facets::ReticleControl>& rc, uint8_t jtag_addr,
	         const bool is_kintex);

private:
	boost::weak_ptr<facets::ReticleControl> mReticleControl;
	const uint8_t m_jtag_addr;
	const bool mKintex;
};

} // namespace Handle
} // namespace HMF
