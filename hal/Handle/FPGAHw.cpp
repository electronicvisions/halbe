#include "FPGAHw.h"
#include "HICANNHw.h"
#include "HMFRun.h"

#include <boost/make_shared.hpp>

#include "hal/Coordinate/HMFGrid.h"
#include "hal/Coordinate/iter_all.h"

#include "spinn_controller.h"
#include "RealtimeComm.h"

namespace HMF {
namespace Handle {

static uint16_t const config_port = 1850;
static uint16_t const pulse_port = 1851;

// hide ugly stuff here
struct FPGAHw::FPGAHandlePIMPL {
	FPGAHandlePIMPL(FPGAHw & f, Coordinate::DNCOnFPGA const d, bool on_wafer, bool kintex, std::set<Coordinate::HICANNOnDNC> hicanns, Coordinate::IPv4 pmu_ip) :
		fpga(f),
		dnc(d),
		fpga_jtag_port(Coordinate::UDPPort(jtag_base_port + d.value())),
		on_wafer(on_wafer),
		kintex(kintex),
		myPowerBackend(
		on_wafer
			? std::unique_ptr<PowerBackend>(new WaferPowerBackend())
			: std::unique_ptr<PowerBackend>(new VerticalSetupPowerBackend())),
		available_hicanns(hicanns),
		pmu_ip(pmu_ip)
	{
		if ((!on_wafer) && (!kintex) && fpga_jtag_port.value() != 1701)
			throw std::runtime_error("Vertical setup supports only DNC 1");
	}

	void init() {
		setup();
		fpga.activate_dnc(dnc);
		create_hicanns();
	}

	void create_hicanns() {
		if (on_wafer) {
			for (auto hicann : Coordinate::iter_all<Coordinate::HICANNOnDNC>()) {
				if (hicann.id() < available_hicanns.size())
					fpga.add_hicann(dnc, hicann);
			}
		} else { // vertical setup
			// On the vertical setup the hicanns are created in hardware dnc channel ordering.
			for (auto hicann = available_hicanns.begin(); hicann != available_hicanns.end(); hicann++) {
				fpga.add_hicann(dnc, *hicann);
			}
		}

	}

	void setup() {
		if (on_wafer) {
			dynamic_cast<WaferPowerBackend&>(*myPowerBackend).SetupReticle(
				fpga.dnc(dnc), fpga, fpga_jtag_port, pmu_ip, highspeed_mode, arq_mode, kintex
			);
		} else { // vertical setup
			dynamic_cast<VerticalSetupPowerBackend&>(*myPowerBackend).SetupReticle(
				fpga.dnc(dnc), fpga, fpga_jtag_port, available_hicanns, highspeed_mode, arq_mode, kintex
			);
		}
	}

	~FPGAHandlePIMPL() {
		myPowerBackend->destroy_reticle(fpga.dnc(dnc));
	}


	Handle::FPGAHw & fpga;
	Coordinate::DNCOnFPGA const dnc;
	Coordinate::UDPPort const fpga_jtag_port;
	bool const on_wafer;
	bool const kintex;
	std::unique_ptr<HMF::PowerBackend> myPowerBackend;
	std::set<Coordinate::HICANNOnDNC> available_hicanns;
	Coordinate::IPv4 pmu_ip;


	// setup defaults
	bool const highspeed_mode = true;
	bool const arq_mode = true;
	static uint16_t const jtag_base_port = 1700;
};
// end of ugly PIMPLed stuff


static bool is_kintex(Coordinate::FPGAGlobal w, bool force_kintex) {
	return force_kintex || w.toWafer().isKintex();
}

bool FPGAHw::HandleParameter::is_kintex(){
	if (setup == Coordinate::SetupType::VSetup || setup == Coordinate::SetupType::FACETSWafer)
		return false;
	return true;
}

bool FPGAHw::HandleParameter::on_wafer(){
	if (setup == Coordinate::SetupType::VSetup || setup == Coordinate::SetupType::CubeSetup)
		return false;
	return true;
}

FPGAHw::FPGAHw(HandleParameter handleparam) :
		FPGA(handleparam.c),
		Base(handleparam.c),
		pimpl(new FPGAHandlePIMPL(*this, handleparam.d, handleparam.on_wafer(),
		                          handleparam.is_kintex(),
		                          handleparam.available_hicanns,
		                          handleparam.pmu_ip)),
		fpga_ip(handleparam.fpga_ip)
{
	pimpl->init();
}

FPGAHw::FPGAHw(Coordinate::FPGAGlobal const c, Coordinate::IPv4 const fpga_ip,
               Coordinate::DNCOnFPGA const d, Coordinate::IPv4 const pmu_ip, bool on_wafer, size_t num_hicanns,
               bool force_kintex) :
		FPGA(c),
		Base(c),
		fpga_ip(fpga_ip)
{
	//FIXME: UGLY but could not find other way to initlize available hicanns set for pimpl
	std::set<Coordinate::HICANNOnDNC> avail_hicanns;
	for (auto hicann : Coordinate::iter_all<Coordinate::HICANNOnDNC>()) {
		if (hicann.id()<num_hicanns)
			avail_hicanns.insert(HMF::Coordinate::HICANNOnDNC(hicann));
	}
	std::unique_ptr<FPGAHandlePIMPL> tempptr(new FPGAHandlePIMPL(*this, d, on_wafer,
                             (on_wafer ? is_kintex(c, force_kintex) : force_kintex),
                             avail_hicanns, pmu_ip));
	pimpl = std::move(tempptr);
	pimpl->init();
}

FPGAHw::~FPGAHw() {}

bool FPGAHw::isKintex() const
{
	return pimpl->kintex;
}

PowerBackend & FPGAHw::getPowerBackend() const {
	return *pimpl->myPowerBackend;
}

// Why no init in ctor?
SpinnController& FPGAHw::get_spinn_controller() const {
	if (!spinn_controller) {
		if (ip() != Coordinate::IPv4()) {
			const_cast<FPGAHw*>(this)->spinn_controller.reset(
					new SpinnController(ip().to_ulong(), config_port, pulse_port));
		} else {
			throw std::runtime_error("FPGA handle has no ip");
		}
	}
	return *spinn_controller.get();
}

RealtimeComm& FPGAHw::get_realtime_comm() const {
	if (!realtime_comm) {
		const_cast<FPGAHw*>(this)->realtime_comm.reset(
				new RealtimeComm(ip().to_string(), /*40000 + pulse_port*/ 55739, pulse_port));
	}
	return *realtime_comm.get();
}

auto FPGAHw::create_hicann(Coordinate::HICANNGlobal const& h) -> hicann_handle_t
{
	Coordinate::DNCGlobal dnc{h.toDNCGlobal(), coordinate().toWafer()};
	if (h.toWafer() != coordinate().toWafer())
		throw std::runtime_error("HICANN is not on the same wafer");

	if (dnc.toDNCOnFPGA() != pimpl->dnc)
	{
		std::stringstream err;
		err << "HICANN " << h << "is on not on " << pimpl->dnc << ", but on " << dnc.toDNCOnFPGA();
		throw std::runtime_error(err.str());
	}

	if (dnc.toFPGAOnWafer() != coordinate())
	{
		std::stringstream err;
		err << "HICANN " << h << "is not on FPGA " << coordinate()
		    << ", but on " << dnc.toFPGAOnWafer();
		throw std::runtime_error(err.str());
	}

	uint8_t jtag_addr =  getPowerBackend().hicann_jtag_addr(h);
	return boost::make_shared<HICANNHw>(
		h, getPowerBackend().get_reticle_ptr(dnc), jtag_addr, isKintex());
}

boost::shared_ptr<facets::ReticleControl> FPGAHw::get_reticle(const Coordinate::DNCOnFPGA & d)
{
	PowerBackend & pb = getPowerBackend();
	Coordinate::DNCGlobal dnc_global(d.toDNCOnWafer(coordinate()), coordinate().toWafer());
	return pb.get_reticle_ptr(dnc_global);
}

boost::shared_ptr<FPGAHw> createFPGAHw(Coordinate::FPGAGlobal const c,
		Coordinate::IPv4 const fpga_ip, Coordinate::DNCOnFPGA const d, Coordinate::IPv4 const pmu_ip,
		bool on_wafer, size_t num_hicanns, bool force_kintex)
{
	boost::shared_ptr<FPGAHw> ptr{new FPGAHw(c, fpga_ip, d, pmu_ip, on_wafer, num_hicanns, force_kintex)};
	return ptr;
}

void freeFPGAHw(boost::shared_ptr<FPGAHw> & handle)
{
	// TODO, can we call garbage collection here?
	if (handle.use_count() != 1)
		throw std::runtime_error("Too many instances of this Handle are out there...");
	handle.reset();
}

} // namespace Handle
} // namespace HMF
