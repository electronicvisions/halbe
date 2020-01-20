#pragma once

#include "hal/Handle/FPGA.h"
#include "hal/Handle/HICANNEss.h"
#include "hal/Handle/Ess.h"

//forward declaration
namespace PyNNParameters {
    struct EIF_cond_exp_isfa_ista;
}

namespace HMF {

namespace Handle {

struct HICANNEss;

struct FPGAEss : public FPGAMixin<HICANNEss>
{
	FPGAEss(halco::hicann::v2::FPGAGlobal const c, boost::shared_ptr<Ess> ess, const std::vector<halco::hicann::v2::HICANNOnWafer> & hicanns);

#ifndef PYPLUSPLUS
	HAL2ESS & ess();
	HAL2ESS const & ess() const;
	Ess & ess_handle();
	Ess const & ess_handle() const;
#endif

	void runESS(long duration_ns);
	void initializeESS();
    PyNNParameters::EIF_cond_exp_isfa_ista getBioParameter(Handle::HICANN const& h, halco::hicann::v2::NeuronOnHICANN const& nrn ) const;
    PyNNParameters::EIF_cond_exp_isfa_ista getTechnicalParameter(Handle::HICANN const& h, halco::hicann::v2::NeuronOnHICANN const& nrn ) const;

private:
#ifndef PYPLUSPLUS
	hicann_handle_t create_hicann(halco::hicann::v2::HICANNGlobal const& h, bool /*request_highspeed*/) override;
#endif
	boost::shared_ptr<Ess> mEss;
};


} // namespace Handle
} // namespace HMF
