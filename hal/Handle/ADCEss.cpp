#include "hal/Handle/ADCEss.h"

#include "ESS/halbe_to_ess.h"

namespace HMF {
namespace Handle {

HAL2ESS & ADCEss::ess()
{
	return *(mEss->getESS());
}

std::pair<halco::hicann::v2::DNCOnWafer, halco::hicann::v2::AnalogOnHICANN>
ADCEss::getESSAnalog() const
{
    //convert the coordinate value from string to int
    const size_t adc_coord_int = std::stoi(coordinate().get_serial());
    halco::hicann::v2::AnalogOnHICANN analog( adc_coord_int % 2);
    halco::common::Enum dncEnum( adc_coord_int / 2);
    return std::make_pair( halco::hicann::v2::DNCOnWafer(dncEnum), analog);
}

HMF::ADC::USBSerial
ADCEss::getVirtualADCCoordinate(halco::hicann::v2::DNCOnWafer dnc, halco::hicann::v2::AnalogOnHICANN analog)
{
    size_t const adc_coord_int      = 2*dnc.toEnum().value() + analog.value();
    std::string const adc_coord_str = std::to_string(adc_coord_int);
    return HMF::ADC::USBSerial(adc_coord_str);
}

bool ADCEss::isESS() const
{
    return true;
}

}	//Handle
}	//HMF
