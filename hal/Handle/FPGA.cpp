#include "FPGA.h"
#include "HICANN.h"

#include <boost/make_shared.hpp>
#include <boost/asio/ip/address_v4.hpp>

#include "hal/Coordinate/HMFGrid.h"
#include "hal/Coordinate/iter_all.h"

namespace HMF {

using namespace Coordinate;

namespace Handle {

FPGA::FPGA(Coordinate::FPGAGlobal const c)
    : coord(c), dnc_coords(), hicann_coords(), m_listen_global(false)
{}


FPGA::~FPGA() {}


bool FPGA::isMaster() const {
	return coord.toFPGAOnWafer() == Coordinate::FPGAOnWafer::Master;
}

bool FPGA::getListenGlobalMode() const
{
	return m_listen_global;
}

void FPGA::setListenGlobalMode(bool const listen)
{
	m_listen_global = listen;
}

size_t FPGA::hicann_count(dnc_coord_t const dnc) const {
	size_t cnt = 0;
	for (auto hicann : Coordinate::iter_all<HMF::Coordinate::HICANNOnDNC>() )
		if (hicann_active(dnc, hicann))
			++cnt;
	return cnt;
}

/// Returns if the given HICANN is active
bool FPGA::hicann_active(dnc_coord_t const dnc, hicann_coord_t const hicann) const
{
	return hicanns[dnc][hicann].get() != nullptr;
}

/// Returns if the given HICANN needs a working highspeed connection
bool FPGA::hicann_highspeed(dnc_coord_t const dnc, hicann_coord_t const hicann) const
{
	return highspeed_hicanns[dnc][hicann].get() != nullptr;
}

/// Returns if the given DNC is activ
bool FPGA::dnc_active(dnc_coord_t const d) const
{
	return active_dncs[d];
}

bool FPGA::operator==(FPGA const& b) const {
	return coordinate() == b.coordinate();
}

auto FPGA::get(dnc_coord_t const& d, hicann_coord_t const& h) -> hicann_handle_t
{
	auto hicann = hicanns.at(d).at(h);
	if (hicann == nullptr)
	{
		std::stringstream err;
		err << "On " << coordinate() << " is the " << h << "(" << d << ") not available";
		throw std::runtime_error(err.str());
	}
	return hicann;
}

auto FPGA::get(const HICANNOnWafer& hicann_local) -> hicann_handle_t
{
	HICANNGlobal hicann(hicann_local, wafer());
	return get(hicann.toDNCOnFPGA(), hicann.toHICANNOnDNC());
}

void FPGA::activate_dnc(const dnc_coord_t & dnc)
{
	active_dncs.set(dnc);
}

void FPGA::add_hicann(const dnc_coord_t & dnc, const hicann_coord_t & h, bool const request_highspeed)
{
	if (!dnc_active(dnc))
		throw std::runtime_error("Attempted to add a HICANN to an inactive DNC");
	hicanns[dnc][h] = create_hicann(hicann(dnc, h));

	if (request_highspeed)
		highspeed_hicanns[dnc][h] = hicanns[dnc][h];
}

} // namespace Handle
} // namespace HMF
