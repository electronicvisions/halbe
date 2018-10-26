#pragma once

#include "hal/Handle/HICANNHw.h"
#include "hal/Handle/FPGA.h"
#include "hal/Coordinate/HMFGeometry.h"
#include "hal/Coordinate/HMFGrid.h"

#include <boost/shared_ptr.hpp>

// fwd decl
struct SpinnController;
struct RealtimeComm;

namespace HMF {
struct PowerBackend;

namespace Handle {

// TODO: make FPGAHw and FPGAVSetup
struct FPGAHw : public FPGAMixin<HICANNHw>
{
	struct HandleParameter {
		Coordinate::FPGAGlobal c;
		Coordinate::IPv4 fpga_ip;
		Coordinate::DNCOnFPGA d;
		//set of hicanns avaiable in JTag chain
		std::set<Coordinate::HICANNOnDNC> physically_available_hicanns;
		Coordinate::SetupType setup;
		Coordinate::IPv4 pmu_ip;

		void check() {
			// TODO
			// check and throw if weired settings
			// to be called check() by FPGAHw ctor or so
		}

		bool is_kintex();
		bool on_wafer();
	};

	FPGAHw(HandleParameter handleparam);
	/// \param num_hicanns number of hicanns on the vertical setup.
	///         ignored if on_wafer = true.
	/// \param force_kintex force to use kintex fpga, otherwise the FPGA is
	///        determined by the FPGA coordinate
	FPGAHw(Coordinate::FPGAGlobal const c, Coordinate::IPv4 const ip,
	       Coordinate::DNCOnFPGA const d, Coordinate::IPv4 const pmu_ip, bool on_wafer = false, size_t num_hicanns = 1,
	       bool force_kintex = false);

	/// Dummy destructor needed by unique_ptr member.
	~FPGAHw();

	/// Returns the ip.
	Coordinate::IPv4 const & ip() const {
		return fpga_ip;
	}

	/// Cast operator to its IP.
	operator Coordinate::IPv4 const & () const {
		return ip();
	}

	bool isKintex() const;

#ifndef PYPLUSPLUS
	// FIXME: vanish!
	PowerBackend & getPowerBackend() const;

	// Gets reticle
	boost::shared_ptr<facets::ReticleControl> get_reticle(const Coordinate::DNCOnFPGA & d);

	// CK/SJ @ECM: This looks fishy why a non const ref from const member function?!
	SpinnController &get_spinn_controller() const;
	RealtimeComm &get_realtime_comm() const;

private:
	struct FPGAHandlePIMPL;
	std::unique_ptr<FPGAHandlePIMPL> pimpl;
	std::shared_ptr<SpinnController> spinn_controller;
	std::shared_ptr<RealtimeComm> realtime_comm;

	hicann_handle_t create_hicann(Coordinate::HICANNGlobal const& h) override;
#endif
private:
	// FPGA ip corresponding to FPGA coordinate
	Coordinate::IPv4 const fpga_ip;
};

// FIXME holy ugliness! looks like a failed attempt to workaround ownership/lifetime issues
boost::shared_ptr<FPGAHw> createFPGAHw(Coordinate::FPGAGlobal const c,
		Coordinate::IPv4 const ip, Coordinate::DNCOnFPGA const d, Coordinate::IPv4 const pmu_ip,
		bool on_wafer=false, size_t num_hicanns=1, bool force_kintex = false);
void freeFPGAHw(boost::shared_ptr<FPGAHw> & handle);

} // namespace Handle
} // namespace HMF
