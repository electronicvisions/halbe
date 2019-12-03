#include "HICANN.h"
#include "FPGA.h"

namespace HMF {
namespace Handle {

HICANN::HICANN(const Coordinate::HICANNGlobal & h, bool request_highspeed) :
	coord(h), m_highspeed(request_highspeed)
{}

/// Equality operator checks coordinates and highspeed
bool HICANN::operator==(HICANN const& b) const {
	return (coordinate() == b.coordinate()) && (highspeed() == b.highspeed());
}

bool HICANN::highspeed() const {
	return m_highspeed;
}

} // end namespace HMF
} // end namespace Handle
