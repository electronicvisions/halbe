#include "hal/Handle/FPGAHw.h"
#include "hal/backend/FPGABackend.h"

#include "RealtimeComm.h"


class HWRealtimeLatencyMeasurementTool {

public :
	HWRealtimeLatencyMeasurementTool(HMF::Handle::FPGAHw &f, halco::hicann::v2::DNCOnFPGA const);

	void configureHardware();
	void setHicannLoopback(HMF::Handle::HICANN &);
	void measuringLoop();

private:

	HMF::Handle::FPGA &f;
	halco::hicann::v2::DNCOnFPGA dnc;
	HMF::Handle::HICANN &h;

	// TODO: only use FPGABackend realtime interface
	RealtimeComm &rc;

	size_t packets;

	std::vector<std::pair<HMF::FPGA::SpinnInputAddress_t, HMF::FPGA::PulseAddress> > addresses;
};
