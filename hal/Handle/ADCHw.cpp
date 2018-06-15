#include "hal/Handle/ADCHw.h"
#include "hal/Handle/ADCRemoteHw.h"

#include "Vmemory.h"
#include "Vusbstatus.h"
#include "Vusbmaster.h"
#include "Vocpfifo.h"
#include "Vmux_board.h"
#include "Vmodule_adc.h"
#include "Vmoduleusb.h"
#include "Vspigyro.h"

#include <sstream>

// (file-)static magic numbers...
static const unsigned int usb_vendorid = 0x04b4;
static const unsigned int usb_deviceid = 0x1003;

static unsigned mux_board_mode(std::string serial)
{
	// this code assumes new frontends are *always* combined with new flyspi,
	// NEVER combine old frontend with new flyspi or the other way around!
	if (serial.size() > 1 && serial[0] == 'B')
		return 2; // new flyspi board, new analog frontend
	else
		return 0; // old flyspi board, old analog frontend
}

// Whitelist of
// Analog breakout board version, bitfile version
static const std::vector<std::pair<int, std::string>> bitfile_whitelist = {
	{1, "1eea2506"},
	{2, "14a549d1"},
	{2, "6ff130"},
	{2, "10d79fae"},
	{2, "1bbb73a"}
};

namespace HMF {
namespace Handle {
	struct ADCHw::VModuleBundle
	{

		// FIXME uh, no delegated ctor in g++-4.6, replicate code for now
		VModuleBundle(std::string serial) :
			io(usbcomm::note, usb_vendorid, usb_deviceid, serial),
			usb(&io),
			status(&usb),
			mem(&usb),
			ocp(&usb),
			mux_board(&ocp, mux_board_mode(serial)),
			adc(&ocp),//,FASTADC__BASEADR)
			gyro(&ocp)
		{
			// bail out if we don't get the matching serial
			if (serial != io.getSerial())
				throw std::invalid_argument("ADC returned wrong serial");
		}


		VModuleBundle() :
			io(usbcomm::note, usb_vendorid, usb_deviceid),
			usb(&io),
			status(&usb),
			mem(&usb),
			ocp(&usb),
			mux_board(&ocp),
			adc(&ocp),//,FASTADC__BASEADR)
			gyro(&ocp)
		{}

		Vmoduleusb io;
		Vusbmaster usb;
		Vusbstatus status;
		Vmemory mem;
		Vocpfifo ocp;
		Vmux_board mux_board;
		Vflyspi_adc adc;
		Vspigyro gyro;
	};

	ADCHw::ADCHw(HMF::ADC::USBSerial const& serial) :
		ADC(serial)
	{
		mBoardId = serial; // FIXME or read from board?
		mComm.reset(new VModuleBundle(serial.get_serial()));
		check_design();
	}

	/* delegate */
	ADCHw::ADCHw(Handle::ADCRemoteHw const& adc) : ADCHw(adc.boardId())
	{
	}

	ADCHw::ADCHw()
	{
		mComm.reset(new VModuleBundle());
		mBoardId = HMF::ADC::USBSerial(mComm->io.getSerial());
		check_design();
	}

	ADCHw::~ADCHw() {
		//mComm.reset(); // not needed as shared_ptr is RAII, and handle is a handle is a handle...
		// Griffe: Zum Abreissen und Wegwerfen!
	}

	HMF::ADC::USBSerial ADCHw::boardId() const {
		return mBoardId;
	}

	Vflyspi_adc & ADCHw::adc() {
		return mComm->adc;
	}

	Vmux_board & ADCHw::mux_board() {
		return mComm->mux_board;
	}

	Vmemory & ADCHw::mem() {
		return mComm->mem;
	}

	Vmoduleusb & ADCHw::usb() {
		return mComm->io;
	}

	Vspigyro & ADCHw::gyro() {
		return mComm->gyro;
	}

	Vusbstatus & ADCHw::status()
	{
		return mComm->status;
	}

	boost::shared_ptr<ADCHw> createADCHw()
	{
		boost::shared_ptr<ADCHw> ptr{new ADCHw{}};
		return ptr;
	}

	boost::shared_ptr<ADCHw> createADCHw(HMF::ADC::USBSerial const& serial)
	{
		boost::shared_ptr<ADCHw> ptr{new ADCHw{serial}};
		return ptr;
	}

	void freeADCHw(boost::shared_ptr<ADCHw> & handle)
	{
		// TODO, can we call garbage collection here?
		if (handle.use_count() != 1)
			throw std::runtime_error("Too many instances of this Handle are out there...");
		handle.reset();
	}

	void ADCHw::check_design()
	{
		std::ostringstream ostr;
		ostr << std::hex << adc().get_version();
		std::string version_string = ostr.str();

		// the following code assumes that flyspi boards
		// with new serial number scheme ("B[0-9]+")
		// are connected to new analog boards, and
		// old serial numbers are connected to old analog
		// boards
		// FIXME: this should be serial number independent,
		// needs a database mapping individual serial number
		// to analog board version
		int version = (boardId().get_serial().substr(0, 1) == "B") ? 2 : 1;
		std::pair<int, std::string> key{version, version_string};
		if (std::find(bitfile_whitelist.begin(), bitfile_whitelist.end(), key)
				== bitfile_whitelist.end()) {
			std::string msg = std::string("wrong ADC design version ");
			msg += version_string + " on board " + boardId().get_serial();
			throw std::runtime_error(msg);
		}
	}

} // namespace Handle
} // namespace ADC
