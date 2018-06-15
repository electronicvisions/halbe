#include "hal/backend/ADCBackend.h"
#include "hal/backend/RemoteADCBackend.h"

#include "hal/Handle/ADC.h"
#include "hal/Handle/ADCRemoteHw.h"

#include "hal/backend/dispatch.h"

#include "Vmux_board.h"
#include "Vmodule_adc.h"
#include "Vmoduleusb.h"
#include "Vusbstatus.h"
#include "Vspigyro.h"
#include "Vmemory.h"
#include "error_base.h"

#include <sstream>
#include <iostream>


using namespace geometry;

static log4cxx::LoggerPtr logger = log4cxx::Logger::getLogger("halbe.backend.adc");

namespace HMF {
namespace ADC {

namespace {
	static const std::vector<Vmux_board::mux_input> mux_lookup = {
	Vmux_board::OUTAMP_0,
	Vmux_board::OUTAMP_1,
	Vmux_board::OUTAMP_2,
	Vmux_board::OUTAMP_3,
	Vmux_board::OUTAMP_4,
	Vmux_board::OUTAMP_5,
	Vmux_board::OUTAMP_6,
	Vmux_board::OUTAMP_7
};
} // end anonymous namespace


// enables dispatcher code path for RemoteHw handles (i.e. ADCRemoteHw here)
#undef USE_REMOTE_HW // general case: empty
#define USE_REMOTE_HW(return, func_name, HandleType, handle, ...) \
	{ \
		if (::HMF::Handle::ADCRemoteHw* rhandle = dynamic_cast< ::HMF::Handle::ADCRemoteHw* >(&handle)) { \
			/* party, it's a remote handle (or a python-wrapper class, use ADC coordinate for RPC) */ \
			return rhandle->adc_client->func_name(EVERYSECOND(_, rhandle->get_usbserial(), __VA_ARGS__)); \
		} \
	}


//***	configure adc board on startup
HALBE_SETTER_GUARDED_WITH_EXCEPTION_TRANSLATION(flyspi::DeviceError,
	EventSetupAnalogReadout,
	config,
	Handle::ADC &, h,
	Config, cfg
) {
	LOG4CXX_TRACE(logger, "config called");
	using namespace std;

	// configure board
	h.mux_board().enable_power();

	//configure mux
	assert(mux_lookup.size() == Coordinate::ChannelOnADC::size - 1);
	assert(Coordinate::ChannelOnADC::GND.value() == -1);
	if (cfg.input() == Coordinate::ChannelOnADC::GND)
		h.mux_board().set_Mux(Vmux_board::MUX_GND);
	else
		h.mux_board().set_Mux( mux_lookup[cfg.input()] );

	const uint32_t startaddr = 0;
	const uint32_t endaddr =
		startaddr + cfg.samples() / 2 + cfg.samples() % 2;

	// write configuration to adc
	h.adc().configure(0);
	// set adc into ready state
	h.adc().setup_controller(
		startaddr, endaddr,
		0 /* single mode */, 0 /* trigger enable */,
		cfg.trigger());
}


HALBE_SETTER_GUARDED_WITH_EXCEPTION_TRANSLATION(flyspi::DeviceError,
	EventStartExperiment,
	prime,
	Handle::ADC &, h
) {
	LOG4CXX_TRACE(logger, "prime called");

	h.adc().set_single_trigger();
}


HALBE_SETTER_GUARDED_WITH_EXCEPTION_TRANSLATION(flyspi::DeviceError,
	EventStartExperiment,
	trigger_now,
	Handle::ADC &, h
) {
	LOG4CXX_TRACE(logger, "trigger_now");
	h.adc().manual_trigger();
}


HALBE_GETTER_WITH_EXCEPTION_TRANSLATION(flyspi::DeviceError,
	Status, get_status,
	Handle::ADC &, h
) {
	LOG4CXX_TRACE(logger, "get_status called");
	Status ret;
	auto status = h.adc().get_status();
	ret.trigger_activated = status.trigger_enabled_bit;
	ret.triggered = status.triggered_bit;
	ret.start_addr = status.start_addr;
	ret.end_addr = status.end_addr;

	std::ostringstream ostr;
	ostr << std::hex << h.adc().get_version();
	ret.version_string = ostr.str();

	return ret;
}


HALBE_GETTER(double, get_sample_rate,
	Handle::ADC &, h
) {
	LOG4CXX_TRACE(logger, "get_sample_rate called");
	return h.status().getUsbClockFrequency();
}

HALBE_GETTER_WITH_EXCEPTION_TRANSLATION(flyspi::DeviceError,
	float, get_temperature,
	Handle::ADC &, h
) {
	LOG4CXX_TRACE(logger, "get_temperature called");
	return h.gyro().read_temperature();
}

HALBE_GETTER_WITH_EXCEPTION_TRANSLATION(flyspi::DeviceError,
	raw_data_type, get_trace,
	Handle::ADC &, h
) {
	// Read datapoints in words of 32bit ≙ 2 samples.
	LOG4CXX_TRACE(logger, "get_trace called");

	const uint32_t addr_offset = 0x08000000;
	const uint32_t startaddr = addr_offset + h.adc().get_startaddr();
	const uint32_t endaddr   = addr_offset + h.adc().get_endaddr();
	const uint32_t num_words = endaddr - startaddr;
	if (endaddr < startaddr)
		throw std::runtime_error("ADC: endaddr < startaddr");

	raw_data_type raw_data(num_words*2);

	// Larger chunks may lead to timeout errors in libusb_bulk_transfer.
	// const unsigned int max_size = 4194304 /* 2^22 */ - 1; // 16MB = 4M 32 bit words
	const uint32_t max_size = 0x40000; // 1024KB = 128K 32 bit words
	for (uint32_t chunk = 0; chunk < num_words; chunk += max_size)
	{
		const uint32_t size = std::min(num_words - chunk, max_size);

		LOG4CXX_TRACE(logger, "read chunk from  " << chunk << " to "
				<< (chunk + size) << " (" << size << " words).")
		Vbufuint_p data = h.mem().readBlock(startaddr + chunk, size);

		const size_t raw_offset = chunk * 2;
		for (unsigned int i = 0; i < size; i++)
		{
			raw_data[raw_offset+i*2  ] = (data[i]>>16)&0xfff;
			raw_data[raw_offset+i*2+1] =  data[i]     &0xfff;
		}
	}

	LOG4CXX_INFO(logger, "received " << raw_data.size() << " samples");
	return raw_data;
}


HALBE_GETTER_WITH_EXCEPTION_TRANSLATION(flyspi::DeviceError,
	USBSerial, get_board_id,
	Handle::ADC &, h
) {
	LOG4CXX_TRACE(logger, "get_board_id called");
	return USBSerial(h.usb().getSerial());
}


} //namespace ADC
} //namespace HMF
