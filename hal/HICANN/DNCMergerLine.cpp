#include "hal/HICANN/DNCMergerLine.h"
#include "halco/hicann/v2/l1.h"

namespace HMF {
namespace HICANN {

const size_t DNCMergerLine::num_merger;

std::ostream& operator<< (std::ostream& o, const DNCMergerLine & dn) {
	static const size_t num_merger = halco::hicann::v2::DNCMergerOnHICANN::size;
	o << "Mergers: " << std::endl;
	for (uint8_t ii = 0; ii < num_merger; ii++) {
		halco::hicann::v2::DNCMergerOnHICANN mer{ii};
		o << mer << ": " << dn[mer] << std::endl;
	}
	return o;
}

halco::hicann::v2::DNCMergerOnHICANN
DNCMergerLine::loopback_target(const halco::hicann::v2::DNCMergerOnHICANN & source) {
	uint8_t tmp = source + ((source % 2) ? -1 : 1);
	return halco::hicann::v2::DNCMergerOnHICANN{tmp};
}

} // HICANN
} // HMF
