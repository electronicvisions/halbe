#include "hal/Handle/PMU.h"

namespace HMF {
namespace Handle {

PMU::PMU() :
	coord(),
	pwr_ip()
{}

PMU::PMU(halco::hicann::v2::IPv4 const & ip) :
	pwr_ip(ip)
{}

PMU::PMU(halco::hicann::v2::PMU const & coord, halco::hicann::v2::IPv4 const & ip) :
	coord(coord),
	pwr_ip(ip)
{}

halco::hicann::v2::IPv4 const & PMU::ip() const {
	return pwr_ip;
}

bool PMU::operator<(PMU const& b) const {
	return coordinate() < b.coordinate();
}

bool PMU::operator==(PMU const& b) const {
	return coordinate() == b.coordinate();
}

PMU::operator halco::hicann::v2::IPv4 const & () const {
	return ip();
}

halco::hicann::v2::PMU PMU::coordinate() const
{
	return coord;
}

} // namespace Handle
} // namespace HMF
