#include "hal/Handle/FPGAHw.h"
#include "hal/backend/FPGABackend.h"
#include "RealtimeComm.h"


class SWRealtimeLatencyMeasurementTool {

public :
	SWRealtimeLatencyMeasurementTool(HMF::Handle::FPGAHw &f, bool const master);

	void measuringLoop();

private:

	HMF::Handle::FPGA &f;
	// TODO: only use FPGABackend realtime interface
	RealtimeComm &rc;
	bool const master;

};
