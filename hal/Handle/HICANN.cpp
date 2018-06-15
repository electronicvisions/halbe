#include "HICANN.h"
#include "FPGA.h"

namespace HMF {
namespace Handle {

HICANN::HICANN(const Coordinate::HICANNGlobal & h) :
	coord(h)
{}

bool HICANN::useScheriff() const {
	return Base().useScheriff();
}

Scheriff& HICANN::get_scheriff() const {
	return Base().get_scheriff();
}

/// Equality operator checks coordinates
bool HICANN::operator==(HICANN const& b) const {
	return coordinate() == b.coordinate();
}


} // end namespace HMF
} // end namespace Handle
