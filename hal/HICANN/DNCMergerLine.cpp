#include "hal/HICANN/DNCMergerLine.h"

namespace HMF {
namespace HICANN {

const size_t DNCMergerLine::num_merger;

std::ostream& operator<< (std::ostream& o, const DNCMergerLine & dn) {
	static const size_t num_merger = Coordinate::DNCMergerOnHICANN::size;
	o << "Mergers: " << std::endl;
	for (uint8_t ii = 0; ii < num_merger; ii++) {
		Coordinate::DNCMergerOnHICANN mer{ii};
		o << mer << ": " << dn[mer] << std::endl;
	}
	return o;
}

Coordinate::DNCMergerOnHICANN
DNCMergerLine::loopback_target(const Coordinate::DNCMergerOnHICANN & source) {
	uint8_t tmp = source + ((source % 2) ? -1 : 1);
	return Coordinate::DNCMergerOnHICANN{tmp};
}

} // HICANN
} // HMF
