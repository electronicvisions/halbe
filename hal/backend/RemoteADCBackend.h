#pragma once

#ifndef PYPLUSPLUS

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include "RCF/RCF.hpp"

#include "hal/backend/ADCBackend.h"

// Fwd decl
namespace HMF {
namespace ADC {
struct USBSerial;
}}

// expose all the HMF::ADC's backend functions via RCF
// ECM: Ask me if(!) you want to change the interface in a backwards-compatible way ;)
// ECM: In general, it's ok to append stuff...
RCF_BEGIN(I_HALbeADC, "I_HALbeADC")
	RCF_METHOD_V2(void,                    config,          HMF::ADC::USBSerial, HMF::ADC::Config)
	RCF_METHOD_R1(double,                  get_sample_rate, HMF::ADC::USBSerial)
	RCF_METHOD_R1(float,                   get_temperature, HMF::ADC::USBSerial)
	RCF_METHOD_V1(void,                    prime,           HMF::ADC::USBSerial)
	RCF_METHOD_V1(void,                    trigger_now,     HMF::ADC::USBSerial)
	RCF_METHOD_R1(HMF::ADC::Status,        get_status,      HMF::ADC::USBSerial)
	RCF_METHOD_R1(HMF::ADC::raw_data_type, get_trace,       HMF::ADC::USBSerial)
	RCF_METHOD_R1(HMF::ADC::USBSerial,    get_board_id,    HMF::ADC::USBSerial)
RCF_END(I_HALbeADC)
#pragma GCC diagnostic pop

#endif // PYPLUSPLUS
