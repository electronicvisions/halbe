#pragma once

#include <boost/shared_ptr.hpp>

#include "hal/ADC/USBSerial.h"
#include "hal/Coordinate/HMFGeometry.h"
#include "hal/Handle/Base.h"

// Fwd Decl
struct Vflyspi_adc;
struct Vmux_board;
struct Vmemory;
struct Vmoduleusb;

namespace HMF {

namespace Handle {

struct ADC : public Base
{
	/// Open connection to specific ADC (identified by its serial number == Coordinate)
	ADC();
	explicit ADC(HMF::ADC::USBSerial const& adc);

	void set_usbserial(HMF::ADC::USBSerial const&);
	HMF::ADC::USBSerial get_usbserial() const;
	HMF::ADC::USBSerial coordinate() const; // needed for hal/Handle/Dump.h

	/// Equality operator checks coordinates
	bool operator==(ADC const& b) const;
	bool operator!=(ADC const& b) const;

	virtual bool isESS() const;

protected:
	HMF::ADC::USBSerial m_usbserial;
};

} // namespace Handle
} // namespace HMF
