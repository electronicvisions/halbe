#pragma once

#include "hal/Handle/HICANN.h"
#include "hal/Handle/FPGAEss.h"
#include "hal/Handle/Ess.h"


namespace HMF {
namespace Handle {

struct FPGAEss;

/**
 * @brief (Ultra-Minimal) Handle class that encapsulates a single HICANN connection to the ess system.
 *
 * @note This will(!) change a lot (issueCmd(), etc.)! Purpose: Adapt/think about parameters/comm hiding.
 */
struct HICANNEss : public HICANN
{
#ifndef PYPLUSPLUS
	HAL2ESS & ess();
#endif

	/// Construct a HICANN that is connected to FPGA f
	explicit HICANNEss(Coordinate::HICANNGlobal const & h, boost::shared_ptr<Ess> ess) :
		HICANN(h),
		mEss(ess)
	{}

	boost::shared_ptr<Ess> mEss;
};

} // namespace Handle
} // namespace HMF
