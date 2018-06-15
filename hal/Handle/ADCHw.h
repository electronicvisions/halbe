#pragma once

#include "hal/Handle/ADC.h"

// Fwd Decl
struct Vflyspi_adc;
struct Vmux_board;
struct Vmemory;
struct Vmoduleusb;
struct Vusbstatus;
struct Vspigyro;

namespace HMF {

namespace Handle {
	struct ADCRemoteHw;
	struct ADCHw;
}

namespace ADC {
	// fwd decls (it's ugly, but including ADCBackend.h is even uglier)
	struct Config;
	typedef uint16_t raw_type;
	typedef std::vector<raw_type> raw_data_type;
	struct Status;
	void configIMPL(Handle::ADCHw &, HMF::ADC::Config);
	void primeIMPL(Handle::ADCHw &);
	void trigger_nowIMPL(Handle::ADCHw &);
	raw_data_type get_traceIMPL(Handle::ADCHw&);
	HMF::ADC::USBSerial get_board_idIMPL(Handle::ADCHw&);
	double get_sample_rateIMPL(Handle::ADCHw & h);
	Status get_statusIMPL(Handle::ADCHw & h);
	float get_temperatureIMPL(Handle::ADCHw & h);
}

namespace Handle {

/**
 * @class ADC
 *
 * @brief Handle class that encapsulates a single ADC connection.
 */
struct ADCHw : public ADC
{
	/// Open connection to specific ADC (identified by its serial number == Coordinate)
	explicit ADCHw(HMF::ADC::USBSerial const& serial);

	/// Construct from remote handle for local usage
	explicit ADCHw(Handle::ADCRemoteHw const& adc);

	/// Open connection to "first free" ADC
	ADCHw();

	/// Close connection to ADC board
	~ADCHw();

	HMF::ADC::USBSerial boardId() const;

private:
#ifndef PYPLUSPLUS
	friend void HMF::ADC::configIMPL(Handle::ADCHw &, HMF::ADC::Config);
	friend void HMF::ADC::primeIMPL(Handle::ADCHw &);
	friend void HMF::ADC::trigger_nowIMPL(Handle::ADCHw &);
	friend HMF::ADC::raw_data_type HMF::ADC::get_traceIMPL(Handle::ADCHw&);
	friend HMF::ADC::USBSerial HMF::ADC::get_board_idIMPL(Handle::ADCHw&);
	friend double HMF::ADC::get_sample_rateIMPL(Handle::ADCHw & h);
	friend HMF::ADC::Status HMF::ADC::get_statusIMPL(Handle::ADCHw & h);
	friend float HMF::ADC::get_temperatureIMPL(Handle::ADCHw & h);

	void check_design();

	Vflyspi_adc & adc();
	Vmux_board & mux_board();
	Vmemory & mem();
	Vmoduleusb & usb();
	Vusbstatus & status();
	Vspigyro & gyro();

	struct VModuleBundle;
	boost::shared_ptr<VModuleBundle> mComm;

	HMF::ADC::USBSerial mBoardId;
#endif
}; // class ADC

boost::shared_ptr<ADCHw> createADCHw();
boost::shared_ptr<ADCHw> createADCHw(HMF::ADC::USBSerial const & adc);
void freeADCHw(boost::shared_ptr<ADCHw> & handle);

} // namespace Handle
} // namespace HMF
