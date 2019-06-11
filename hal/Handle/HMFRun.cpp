#include "hal/Handle/HMFRun.h"

#include <stdexcept>

#include "hal/Handle/HICANNHw.h"
#include "hal/Handle/FPGAHw.h"

#include "hal/Coordinate/HMFGrid.h"
#include "hal/Coordinate/iter_all.h"

#include "reticle_control.h"

using std::runtime_error;
using facets::ReticleControl;
using namespace HMF::Coordinate;
using namespace facets;
using namespace std;

namespace HMF {

///PowerBackend stuff
PowerBackend::PowerBackend() : all_reticles() {}

PowerBackend::~PowerBackend() {}

HostALController&
PowerBackend::get_host_al(Handle::FPGAHw const& f) {
	// FIXME getting access to the HostAL should not go via reticle control
	ReticleControl& reticle = get_some_reticle(f);
	if (dynamic_cast<S2C_JtagPhys2FpgaArq*>(reticle.jtag_p2f.get())) // keep arq in reset
		return *(dynamic_cast<S2C_JtagPhys2FpgaArq*>(reticle.jtag_p2f.get())->getHostAL());
	else
		throw std::runtime_error("cannot access HostAL, S2C_JtgPhys2FPGA runs without ARQ");
}


template<typename T /*std::pair*/>
struct CompareFirstOfPair {
	CompareFirstOfPair(typename T::first_type val) :
		val(val)
	{}

	bool operator()(T const & elem) const {
		return val == elem.first;
	}
private:
	typename T::first_type val;
};

boost::shared_ptr<facets::ReticleControl> PowerBackend::get_reticle_ptr(Coordinate::DNCGlobal const d) {
	auto it = std::find_if(all_reticles.begin(), all_reticles.end(), CompareFirstOfPair<decltype(all_reticles)::value_type>(d));
	if (it == all_reticles.end()) {
		std::stringstream ss;
		ss << "Could not find matching reticle: " << d << std::endl;
		throw std::runtime_error(ss.str());
	}
	return it->second;
}

ReticleControl& PowerBackend::get_reticle(Coordinate::DNCGlobal const d) {
	return *get_reticle_ptr(d);
}

ReticleControl& PowerBackend::get_reticle(Handle::FPGAHw const & f, Coordinate::DNCOnFPGA const d) {
	return get_reticle(f.dnc(d));
}

ReticleControl& PowerBackend::get_some_reticle(Handle::FPGAHw const & f) {
	for (size_t i = Coordinate::DNCOnFPGA::min; i <= Coordinate::DNCOnFPGA::max; i++) {
		Coordinate::DNCOnFPGA const d{Enum(i)};
		auto it = std::find_if(all_reticles.begin(), all_reticles.end(), CompareFirstOfPair<decltype(all_reticles)::value_type>(f.dnc(d)));
		if (it != all_reticles.end())
			return *(it->second);
	}
	std::stringstream ss;
	ss << "Could not find any reticle from FPGA: " << f.coordinate() << std::endl;
	throw std::runtime_error(ss.str());
}

void PowerBackend::destroy_reticle(Coordinate::DNCGlobal const d) {
	auto it = std::find_if(all_reticles.begin(), all_reticles.end(), CompareFirstOfPair<decltype(all_reticles)::value_type>(d));
	if (it == all_reticles.end()) {
		std::stringstream ss;
		ss << "Cannot destroy reticle because it doesn't exist: ";
		ss << d << " (" << d.toFPGAGlobal() << ")\n";
		ss << "Existing reticles: ";
		for (auto & r: all_reticles)
			ss << r.first << "\n";
		Logger & log = Logger::instance();
		log(Logger::WARNING) << ss.str() << Logger::flush;
	} else {
		if (it->second.use_count() != 1)
		{
			std::stringstream ss;
			ss << "Cannot destroy reticle '"
			   << d << " (" << d.toFPGAGlobal()
			   << ")', because it is still used somewhere" << std::endl;
			throw std::runtime_error(ss.str());
		}
		all_reticles.erase(it);
	}
}

uint8_t PowerBackend::hicann_jtag_addr(Coordinate::HICANNGlobal const& h) {
	//look up in ReticleControl how many HICANNs it has
	uint8_t hs_channel = h.toHICANNOnDNC().toHighspeedLinkOnDNC().toEnum();
	assert (hs2jtag_lut.count(hs_channel) > 0);
	return hs2jtag_lut[hs_channel];
}

// single reticle usage
void PowerBackend::SetupReticle(
    Coordinate::DNCGlobal const d,
    Coordinate::IPv4 fpga_ip,
    uint16_t jtag_port,
    Coordinate::IPv4 pmu_ip,
    std::set<Coordinate::HICANNOnDNC> physically_available_hicanns,
    std::set<Coordinate::HICANNOnDNC> highspeed_hicanns,
    bool on_wafer,
    bool arq_mode,
    Coordinate::JTAGFrequency jtag_freq)
{
	if (all_reticles.size() > 1) {
		throw runtime_error("PowerBackend::SetupReticle: Too many reticles instantiated!");
	}
	if (all_reticles.size()) {
		Logger & log = Logger::instance();
		log(Logger::WARNING) << "Ignoring request to create another reticle instance.\n";
		log(Logger::WARNING) << "Reticles in use: ";
		for (auto & r: all_reticles) {
			log(Logger::WARNING) << r.first << " ";
		}
		log(Logger::WARNING) << std::endl << Logger::flush;

		return;
	}

	// creation of array and bitset for reticlecontrol, also gets used to keep which hicanns are
	// availabe in hs channel ordering
	size_t jtag_num = physically_available_hicanns.size();
	std::bitset<Coordinate::HighspeedLinkOnDNC::end> avail_hicann_bitset_in_hs_order;
	std::bitset<Coordinate::HICANNOnDNC::enum_type::end> highspeed_bitset;
	for (auto const hs_link : Coordinate::iter_all<Coordinate::HighspeedLinkOnDNC>()) {
		if (physically_available_hicanns.count(hs_link.toHICANNOnDNC())) {
			avail_hicann_bitset_in_hs_order.set(hs_link.toEnum());
			hs2jtag_lut[hs_link.toEnum()] = --jtag_num;
		}
		if (highspeed_hicanns.count(hs_link.toHICANNOnDNC())) {
			highspeed_bitset.set(hs_link.toEnum());
		}
	}

	auto const bytes = fpga_ip.to_bytes();
	ReticleControl::ip_t ip_(bytes[0], bytes[1], bytes[2], bytes[3]);
	unsigned int const reticle_number = d.toDNCOnWafer().toEnum();
	auto ret = all_reticles.insert(std::make_pair(
	    d, boost::shared_ptr<ReticleControl>(new ReticleControl(
	           reticle_number, ip_, jtag_port, pmu_ip, avail_hicann_bitset_in_hs_order,
	           highspeed_bitset, on_wafer,
	           arq_mode, jtag_freq.value() / 1e3))));
	if (!ret.second) {
		throw runtime_error("PowerBackend::SetupReticle: Could not insert reticle");
	}
}

} // namespace HMF
