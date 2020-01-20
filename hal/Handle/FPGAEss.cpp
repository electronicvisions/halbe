#include "FPGAEss.h"

#include <cassert>
#include <boost/make_shared.hpp>

#include "halco/common/iter_all.h"
#include "ESS/halbe_to_ess.h"
#include "euter/cellparameters.h"

namespace HMF {
namespace Handle {

FPGAEss::FPGAEss(halco::hicann::v2::FPGAGlobal const c, boost::shared_ptr<Ess> ess, const std::vector<halco::hicann::v2::HICANNOnWafer> & hicanns) :
	FPGA(c),
	Base(c),
	mEss(ess)
{
	assert(wafer() == mEss->wafer());

	for (auto dnc : halco::common::iter_all<halco::hicann::v2::DNCOnFPGA>())
	{
		activate_dnc(dnc);
	}
	for( auto hicann_local : hicanns )
	{
		halco::hicann::v2::HICANNGlobal hicann(hicann_local, wafer());
		if (hicann.toFPGAOnWafer() != coordinate())
		{
			std::stringstream ss;
			ss << "HICANN: " << hicann << " does not correspond to FPGA: " << coordinate();
			throw std::runtime_error(ss.str());
		}
		add_hicann(hicann.toDNCOnFPGA(),
		           hicann.toHICANNOnDNC());
		ess->add_hicann(hicann);
    }
}

auto FPGAEss::create_hicann(halco::hicann::v2::HICANNGlobal const& h, bool /*request_highspeed*/) -> hicann_handle_t
{
	return boost::make_shared<HICANNEss>(h, mEss);
}

HAL2ESS & FPGAEss::ess()
{
	return *(mEss->getESS());
}

void FPGAEss::runESS(long duration_ns)
{
	mEss->run(duration_ns);	
}

void FPGAEss::initializeESS()
{
	mEss->initialize();
}

HAL2ESS const & FPGAEss::ess() const
{
	return *(mEss->getESS());
}

Ess & FPGAEss::ess_handle()
{
	return *mEss;
}

Ess const & FPGAEss::ess_handle() const
{
	return *mEss;
}

PyNNParameters::EIF_cond_exp_isfa_ista FPGAEss::getBioParameter(Handle::HICANN const& h, halco::hicann::v2::NeuronOnHICANN const& nrn ) const
{
    return ess().get_bio_parameter(h,nrn);
}

PyNNParameters::EIF_cond_exp_isfa_ista FPGAEss::getTechnicalParameter(Handle::HICANN const& h, halco::hicann::v2::NeuronOnHICANN const& nrn ) const
{
    return ess().get_technical_parameter(h,nrn);
}

} // namespace Handle
} // namespace HMF
