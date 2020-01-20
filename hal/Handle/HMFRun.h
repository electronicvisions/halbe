// Copyleft; i don't like header headers
#pragma once

#include <bitset>
#include <map>
#include <string>
#include <set>

#include "pywrap/compat/cstdint.hpp"
#include "pywrap/compat/macros.hpp"

#include "halco/hicann/v2/external.h"

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
	typedef std::map<halco::hicann::v2::DNCGlobal, boost::shared_ptr<facets::ReticleControl> > container_type;
	container_type all_reticles;

	//returns a shared_ptr to the correct reticle
	boost::shared_ptr<facets::ReticleControl> get_reticle_ptr(halco::hicann::v2::DNCGlobal const d);

	//returns a reference to the correct reticle
	facets::ReticleControl& get_reticle(halco::hicann::v2::DNCGlobal const d);

	//returns a reference to the correct reticle
	facets::ReticleControl& get_reticle(Handle::FPGAHw const& f, halco::hicann::v2::DNCOnFPGA const d);

	//returns a reference to one of the reticles that a FPGA f handles
	facets::ReticleControl& get_some_reticle(Handle::FPGAHw const& f);

	//switches reticle off and deletes it. returns true on success
	//must be implemented later because all HICANNBackends of the reticle
	//have to be deleted beforehand and this is not yet possible (stateless)
	// TODO: fix descriptions (ECM)
	void destroy_reticle(halco::hicann::v2::DNCGlobal const d);

	//converts HICANN coordinate in JTAG-relevant reticle-intern HICANN number
	uint8_t hicann_jtag_addr(halco::hicann::v2::HICANNGlobal const& h);

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
	    halco::hicann::v2::DNCGlobal const d,
	    halco::hicann::v2::IPv4 fpga_ip,
	    uint16_t jtag_port,
	    halco::hicann::v2::IPv4 pmu_ip,
	    std::set<halco::hicann::v2::HICANNOnDNC> physically_available_hicanns,
	    std::set<halco::hicann::v2::HICANNOnDNC> highspeed_hicanns,
	    bool on_wafer,
	    bool arq_mode = true,
	    halco::hicann::v2::JTAGFrequency jtag_freq = halco::hicann::v2::JTAGFrequency());
};
#endif // #ifndef PYPLUSPLUS

} // namespace HMF
