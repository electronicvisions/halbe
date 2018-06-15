#include "DNCContainer.h"
#include "hal/Coordinate/iter_all.h"
#include "hal/macro_HALbe.h"


namespace HMF {
namespace DNC {

GbitLink& GbitReticle::operator[] (Coordinate::HICANNOnDNC const & hicann_on_reticle) {
	return links[hicann_on_reticle.id()];
}
GbitLink const & GbitReticle::operator[] (Coordinate::HICANNOnDNC const & hicann_on_reticle) const {
	return links[hicann_on_reticle.id()];
}
bool GbitReticle::operator==(const GbitReticle & other) const {
	return COMPARE_EQUAL(other, links);
}

std::ostream& operator<< (std::ostream& o, GbitReticle const& gr) {
	o << "GbitReticle:\n";
	for (auto hicann : Coordinate::iter_all<HMF::Coordinate::HICANNOnDNC>() ) {
		o << hicann << ":\n";
		o << gr[hicann];
	}
	return o;
}

bool& Loopback::operator[] (Coordinate::HICANNOnDNC const & hicann_on_reticle) {
	return data[hicann_on_reticle.id()];
}
bool const& Loopback::operator[] (Coordinate::HICANNOnDNC const & hicann_on_reticle) const {
	return data[hicann_on_reticle.id()];
}
bool Loopback::operator==(const Loopback & other) const {
	return COMPARE_EQUAL(other, data);
}

std::ostream& operator<< (std::ostream& o, Loopback const& l) {
	o << "Loopback:\n";
	for (auto hicann : Coordinate::iter_all<HMF::Coordinate::HICANNOnDNC>() ) {
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


