#include <gtest/gtest.h>

#include "hal/ADC/USBSerial.h"


TEST(ADC, USBSerial)
{
	HMF::ADC::USBSerial serial1;
	serial1.set_serial("B123456");
	ASSERT_EQ("B123456", serial1.get_serial());

	HMF::ADC::USBSerial serial2 = serial1;
	HMF::ADC::USBSerial serial3 = HMF::ADC::USBSerial("B654321");

	ASSERT_TRUE(serial1 == serial2);
	ASSERT_FALSE(serial1 == serial3);
	ASSERT_TRUE(serial1 != serial3);
	ASSERT_FALSE(serial1 != serial2);

}
