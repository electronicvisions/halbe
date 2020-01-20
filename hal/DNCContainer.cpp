#include "DNCContainer.h"
#include "halco/common/iter_all.h"
#include "hal/macro_HALbe.h"

#include "halco/hicann/v2/hicann.h"

namespace HMF {
namespace DNC {

GbitLink& GbitReticle::operator[] (halco::hicann::v2::HICANNOnDNC const & hicann_on_reticle) {
	return links[hicann_on_reticle.toEnum()];
}
GbitLink const & GbitReticle::operator[] (halco::hicann::v2::HICANNOnDNC const & hicann_on_reticle) const {
	return links[hicann_on_reticle.toEnum()];
}
bool GbitReticle::operator==(const GbitReticle & other) const {
	return COMPARE_EQUAL(other, links);
}

std::ostream& operator<< (std::ostream& o, GbitReticle const& gr) {
	o << "GbitReticle:\n";
	for (auto hicann : halco::common::iter_all<halco::hicann::v2::HICANNOnDNC>() ) {
		o << hicann << ":\n";
		o << gr[hicann];
	}
	return o;
}

bool& Loopback::operator[] (halco::hicann::v2::HICANNOnDNC const & hicann_on_reticle) {
	return data[hicann_on_reticle.toEnum()];
}
bool const& Loopback::operator[] (halco::hicann::v2::HICANNOnDNC const & hicann_on_reticle) const {
	return data[hicann_on_reticle.toEnum()];
}
bool Loopback::operator==(const Loopback & other) const {
	return COMPARE_EQUAL(other, data);
}

std::ostream& operator<< (std::ostream& o, Loopback const& l) {
	o << "Loopback:\n";
	for (auto hicann : halco::common::iter_all<halco::hicann::v2::HICANNOnDNC>() ) {
		o << hicann << " : " << l[hicann] << "\n";
	}
	return o;
}

void Status::check(){
	if (getHardwareId()!= 0x1474346f) throw std::runtime_error("DNC ID invalid");
	//~ if (getCRCCount()) throw std::runtime_error("DNC CRC counter is not zero");
}

} // namespace DNC
} // namespace HMF


