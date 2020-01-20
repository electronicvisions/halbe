#pragma once

#include "hal/Handle/ADC.h"
#include "hal/Handle/Ess.h"
#include "hal/ADC/USBSerial.h"

namespace HMF {
//forward declaration
    class HAL2ESS;

namespace Handle {

struct ADCEss : public ADC
{
    /// Open connection to specific ADC (identified by its serial number == Coordinate)
    explicit ADCEss(HMF::ADC::USBSerial const & adc, boost::shared_ptr<Ess> ess) :
        ADC(adc),
        mEss(ess)
    {}

#ifndef PYPLUSPLUS
	HAL2ESS & ess();
#endif

    std::pair<halco::hicann::v2::DNCOnWafer, halco::hicann::v2::AnalogOnHICANN>
    getESSAnalog() const;

    static HMF::ADC::USBSerial
    getVirtualADCCoordinate(halco::hicann::v2::DNCOnWafer dnc, halco::hicann::v2::AnalogOnHICANN analog);

    virtual bool isESS() const;
private:
	boost::shared_ptr<Ess> mEss;
};

} // namespace Handle
} // namespace HMF

