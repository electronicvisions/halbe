#pragma once

#include <vector>

#include "halco/hicann/v2/fwd.h"
#include "hal/ADC/Config.h"
#include "hal/ADC/Status.h"
#include "hal/ADC/USBSerial.h"

// Fwd decl
namespace HMF {
namespace Handle {
struct ADC;
}}

namespace HMF {
namespace ADC {



	typedef uint16_t raw_type;
	typedef std::vector<raw_type> raw_data_type;

	/**
		* Configure ADC board
		*/
	void config(Handle::ADC & h, Config cfg);

	/**
	 * Read out the sample rate of the adc board
	 */
	double get_sample_rate(Handle::ADC & h);

	/**
	 * Read out the temperature from the gyroscope
	 */
	float get_temperature(Handle::ADC & h);

	/**
		* Prime ADC to start measurement upon next trigger signal
		*/
	void prime(Handle::ADC & h);

	/**
		* Trigger on demand
		*/
	void trigger_now(Handle::ADC & h);

	Status get_status(Handle::ADC & h);

	/**
		* Reads out ADC data and returns data.
		*
		* @return ADC trace data
		*/
	raw_data_type get_trace(Handle::ADC & h);

	USBSerial get_board_id(Handle::ADC& h);

	/**
		* Converts raw ADC data to voltages
		*/
//	voltage_data_type convert_raw_data(Handle::ADC &h, raw_data_type const& raw_data);

	/* when changing the ::HMF::ADC API, please have a look on the RemoteADCBackend! */
} //namespace ADC

} // namespace HMF
