#include "hal/Handle/PMU.h"

namespace HMF {
namespace Handle {

PMU::PMU() :
	coord(),
	pwr_ip()
{}

PMU::PMU(Coordinate::IPv4 const & ip) :
	pwr_ip(ip)
{}

PMU::PMU(Coordinate::PMU const & coord, Coordinate::IPv4 const & ip) :
	coord(coord),
	pwr_ip(ip)
{}

Coordinate::IPv4 const & PMU::ip() const {
	return pwr_ip;
}

bool PMU::operator<(PMU const& b) const {
	return coordinate() < b.coordinate();
}

bool PMU::operator==(PMU const& b) const {
	return coordinate() == b.coordinate();
}

PMU::operator Coordinate::IPv4 const & () const {
	return ip();
}

Coordinate::PMU PMU::coordinate() const
{
	return coord;
}

} // namespace Handle
} // namespace HMF
