// Copyleft; i don't like header headers
#pragma once

#include <bitset>
#include <map>
#include <string>

#include "pywrap/compat/cstdint.hpp"
#include "pywrap/compat/macros.hpp"

#include "hal/Coordinate/HMFGeometry.h"

#ifndef PYPLUSPLUS
#include <boost/shared_ptr.hpp>
#include <memory> // for unique_ptr
#endif

// Fwd Decl
namespace facets {
	struct ReticleControl;
}

struct HostALController;

namespace HMF {
// fwd decl
namespace Handle {
struct FPGAHw;
struct HICANNHw;
}


// TODO Erik think about this!!!! Carefully!!!
// quoting Rincewind: "Multiple exclamation marks are a sure sign of a diseased mind."



// rest hidden from py++
#ifndef PYPLUSPLUS
/**
 * @class PowerBackend
 *
 * @brief Power and coordinate management of the reticlesand corresponding
 * HICANNs
 *
 * @note Potentially intended to manage many reticles. For now only one as
 * the release does not foresee to use more than one HICANN
 */
struct PowerBackend {
	typedef std::map<Coordinate::DNCGlobal, boost::shared_ptr<facets::ReticleControl> > container_type;
	container_type all_reticles;

	//returns a shared_ptr to the correct reticle
	boost::shared_ptr<facets::ReticleControl> get_reticle_ptr(Coordinate::DNCGlobal const d);

	//returns a reference to the correct reticle
	facets::ReticleControl& get_reticle(Coordinate::DNCGlobal const d);

	//returns a reference to the correct reticle
	facets::ReticleControl& get_reticle(Handle::FPGAHw const& f, Coordinate::DNCOnFPGA const d);

	//returns a reference to one of the reticles that a FPGA f handles
	facets::ReticleControl& get_some_reticle(Handle::FPGAHw const& f);

	//switches reticle off and deletes it. returns true on success
	//must be implemented later because all HICANNBackends of the reticle
	//have to be deleted beforehand and this is not yet possible (stateless)
	// TODO: fix descriptions (ECM)
	void destroy_reticle(Coordinate::DNCGlobal const d);

	//converts HICANN coordinate in JTAG-relevant reticle-intern HICANN number
	uint8_t hicann_jtag_addr(Coordinate::HICANNGlobal const& h);

	/// returns a reference to the Host Application Layer for the given FPGA.
	HostALController& get_host_al(Handle::FPGAHw const& f);

	// as unique_ptr calls it...
	~PowerBackend();

protected:
	PowerBackend();

	// LUT to assign jtag index to hs channel index
	std::map<unsigned int, unsigned int> hs2jtag_lut;

private:
	PowerBackend(PowerBackend const &) = delete;

	friend class ::HMF::Handle::FPGAHw;
	void SetupReticle(
	    Coordinate::DNCGlobal const d,
	    Coordinate::IPv4 fpga_ip,
	    uint16_t jtag_port,
	    Coordinate::IPv4 pmu_ip,
	    std::set<Coordinate::HICANNOnDNC> physically_available_hicanns,
	    std::set<Coordinate::HICANNOnDNC> highspeed_hicanns,
	    bool on_wafer,
	    bool arq_mode = true,
	    Coordinate::JTAGFrequency jtag_freq = Coordinate::JTAGFrequency());
};
#endif // #ifndef PYPLUSPLUS

} // namespace HMF
