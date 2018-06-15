#include "hal/Handle/ADC.h"

namespace HMF {
namespace Handle {

ADC::ADC() : m_usbserial("")
{}

ADC::ADC(HMF::ADC::USBSerial const& adc) : m_usbserial(adc)
{}

void ADC::set_usbserial(HMF::ADC::USBSerial const& usbserial)
{
	m_usbserial = usbserial;
}

HMF::ADC::USBSerial ADC::get_usbserial() const
{
	return m_usbserial;
}

HMF::ADC::USBSerial ADC::coordinate() const
{
	return m_usbserial;
}

bool ADC::operator==(ADC const& b) const
{
	return get_usbserial() == b.get_usbserial();
}

bool ADC::operator!=(ADC const& b) const
{
	return !(*this == b);
}

bool ADC::isESS() const
{
	return false;
}

} // namespace Handle
} // namespace ADC
