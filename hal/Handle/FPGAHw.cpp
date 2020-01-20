#include "FPGAHw.h"
#include "HICANNHw.h"
#include "HMFRun.h"

#include <boost/make_shared.hpp>

#include "hal/Coordinate/HMFGrid.h"
#include "halco/common/iter_all.h"
#include "halco/hicann/v2/format_helper.h"

#include "spinn_controller.h"
#include "RealtimeComm.h"

#include "slurm/vision_defines.h"

namespace HMF {
namespace Handle {

static uint16_t const config_port = 1850;
static uint16_t const pulse_port = 1851;

// hide ugly stuff here
struct FPGAHw::FPGAHandlePIMPL {
	FPGAHandlePIMPL(
	    FPGAHw& f,
	    halco::hicann::v2::DNCOnFPGA const d,
	    bool on_wafer,
	    std::set<halco::hicann::v2::HICANNOnDNC> physically_available_hicanns,
	    std::set<halco::hicann::v2::HICANNOnDNC> highspeed_hicanns,
	    std::set<halco::hicann::v2::HICANNOnDNC> usable_hicanns,
	    halco::hicann::v2::IPv4 pmu_ip,
	    halco::hicann::v2::JTAGFrequency jtag_frequency)
	    : fpga(f),
	      dnc(d),
	      fpga_jtag_port(halco::hicann::v2::UDPPort(jtag_base_port + d.value())),
	      on_wafer(on_wafer),
	      myPowerBackend(std::unique_ptr<PowerBackend>(new PowerBackend())),
	      physically_available_hicanns(physically_available_hicanns),
	      highspeed_hicanns(highspeed_hicanns),
	      usable_hicanns(usable_hicanns),
	      pmu_ip(pmu_ip),
	      jtag_frequency(jtag_frequency)
	{}

	void init() {
		if (!fpga.license_valid()) {
			throw std::runtime_error("Missing license for " + halco::hicann::v2::short_format(fpga.coordinate()));
		}
		setup();
		fpga.activate_dnc(dnc);
		create_hicanns();
	}

	void create_hicanns() {
		for (auto hicann : usable_hicanns) {
			bool const needs_highspeed = highspeed_hicanns.count(hicann);
			fpga.add_hicann(dnc, hicann, needs_highspeed);
		}
	}

	void setup() {
		myPowerBackend->SetupReticle(
		    fpga.dnc(dnc), fpga, fpga_jtag_port, pmu_ip, physically_available_hicanns,
		    highspeed_hicanns, on_wafer, arq_mode, jtag_frequency);
	}

	~FPGAHandlePIMPL() {
		myPowerBackend->destroy_reticle(fpga.dnc(dnc));
	}


	Handle::FPGAHw & fpga;
	halco::hicann::v2::DNCOnFPGA const dnc;
	halco::hicann::v2::UDPPort const fpga_jtag_port;
	bool const on_wafer;
	std::unique_ptr<HMF::PowerBackend> myPowerBackend;
	std::set<halco::hicann::v2::HICANNOnDNC> physically_available_hicanns;
	std::set<halco::hicann::v2::HICANNOnDNC> highspeed_hicanns;
	std::set<halco::hicann::v2::HICANNOnDNC> usable_hicanns;
	halco::hicann::v2::IPv4 pmu_ip;


	// setup defaults
	bool const highspeed_mode = true;
	bool const arq_mode = true;
	static uint16_t const jtag_base_port = 1700;
	halco::hicann::v2::JTAGFrequency jtag_frequency;
};
// end of ugly PIMPLed stuff

bool FPGAHw::HandleParameter::on_wafer(){
	if (setup == halco::hicann::v2::SetupType::VSetup || setup == halco::hicann::v2::SetupType::CubeSetup)
		return false;
	return true;
}

FPGAHw::FPGAHw(HandleParameter handleparam)
    : FPGA(handleparam.c),
      Base(handleparam.c),
      pimpl(new FPGAHandlePIMPL(
          *this,
          handleparam.d,
          handleparam.on_wafer(),
          handleparam.physically_available_hicanns,
          handleparam.highspeed_hicanns,
          handleparam.usable_hicanns,
          handleparam.pmu_ip,
          handleparam.jtag_frequency)),
      fpga_ip(handleparam.fpga_ip)
{
	pimpl->init();
}

FPGAHw::FPGAHw(halco::hicann::v2::FPGAGlobal const c, halco::hicann::v2::IPv4 const fpga_ip,
               halco::hicann::v2::DNCOnFPGA const d, halco::hicann::v2::IPv4 const pmu_ip, bool on_wafer, size_t num_hicanns,
               halco::hicann::v2::JTAGFrequency jtag_frequency) :
		FPGA(c),
		Base(c),
		fpga_ip(fpga_ip),
		jtag_frequency(jtag_frequency)
{
	//FIXME: UGLY but could not find other way to initlize available hicanns set for pimpl
	std::set<halco::hicann::v2::HICANNOnDNC> avail_hicanns;
	for (auto hicann : halco::common::iter_all<halco::hicann::v2::HICANNOnDNC>()) {
		if (hicann.toEnum()<num_hicanns)
			avail_hicanns.insert(halco::hicann::v2::HICANNOnDNC(hicann));
	}
	// FIXME: does not support non-highspeed hicanns
	std::unique_ptr<FPGAHandlePIMPL> tempptr(new FPGAHandlePIMPL(
	    *this, d, on_wafer, avail_hicanns,
	    avail_hicanns, avail_hicanns, pmu_ip, jtag_frequency));
	pimpl = std::move(tempptr);
	pimpl->init();
}

FPGAHw::~FPGAHw() {}

PowerBackend & FPGAHw::getPowerBackend() const {
	return *pimpl->myPowerBackend;
}

// Why no init in ctor?
SpinnController& FPGAHw::get_spinn_controller() const {
	if (!spinn_controller) {
		if (ip() != halco::hicann::v2::IPv4()) {
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

auto FPGAHw::create_hicann(halco::hicann::v2::HICANNGlobal const& h, bool request_highspeed) -> hicann_handle_t
{
	halco::hicann::v2::DNCGlobal dnc{h.toDNCGlobal(), coordinate().toWafer()};
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
	return boost::make_shared<HICANNHw>(h, getPowerBackend().get_reticle_ptr(dnc), jtag_addr, request_highspeed);
}

boost::shared_ptr<facets::ReticleControl> FPGAHw::get_reticle(const halco::hicann::v2::DNCOnFPGA & d)
{
	PowerBackend & pb = getPowerBackend();
	halco::hicann::v2::DNCGlobal dnc_global(d.toDNCOnWafer(coordinate()), coordinate().toWafer());
	return pb.get_reticle_ptr(dnc_global);
}

boost::shared_ptr<FPGAHw> createFPGAHw(halco::hicann::v2::FPGAGlobal const c,
		halco::hicann::v2::IPv4 const fpga_ip, halco::hicann::v2::DNCOnFPGA const d, halco::hicann::v2::IPv4 const pmu_ip,
		bool on_wafer, size_t num_hicanns, halco::hicann::v2::JTAGFrequency jtag_frequency)
{
	boost::shared_ptr<FPGAHw> ptr{new FPGAHw(c, fpga_ip, d, pmu_ip, on_wafer, num_hicanns, jtag_frequency)};
	return ptr;
}

void freeFPGAHw(boost::shared_ptr<FPGAHw> & handle)
{
	// TODO, can we call garbage collection here?
	if (handle.use_count() != 1)
		throw std::runtime_error("Too many instances of this Handle are out there...");
	handle.reset();
}

std::optional<FPGA::license_t> FPGAHw::expected_license() const
{
	return halco::hicann::v2::slurm_license(coordinate());
}

bool FPGAHw::license_valid() const
{
	std::string const hardware_licenses = (std::getenv(vision_slurm_hardware_licenses_env_name) != nullptr)
	                                          ? std::getenv(vision_slurm_hardware_licenses_env_name)
	                                          : "";
	return hardware_licenses.find(expected_license().value()) != std::string::npos;
}

} // namespace Handle
} // namespace HMF
