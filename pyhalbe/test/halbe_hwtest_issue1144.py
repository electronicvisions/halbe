#!/usr/bin/env python

import pyhalbe
from HWTest import HWTest


class ADCStabilityTest(HWTest):
    def setUp(self):
        super(ADCStabilityTest, self).setUp()
        self.handle_adc = pyhalbe.Handle.createADCHw()
        self.addCleanup(pyhalbe.Handle.freeADCHw, self.handle_adc)

    def test_ManySamplesTest(self):
        """Record and transfer many ADC samples to provoke USB transfer errors."""

        handle_adc = self.handle_adc
        coord_trig = pyhalbe.Coordinate.TriggerOnADC(0)
        nSamples = 10**8  # many samples
        # run once on each channel for more transfers
        for chan in range(8):
            coord_chan = pyhalbe.Coordinate.ChannelOnADC(chan)
            conf = pyhalbe.ADC.Config(nSamples, coord_chan, coord_trig)
            pyhalbe.ADC.config(handle_adc, conf)

            pyhalbe.ADC.trigger_now(handle_adc)
            trace = pyhalbe.ADC.get_trace(handle_adc)
            self.assertGreater(len(trace), 0)  # non-empty


if __name__ == '__main__':
    ADCStabilityTest.main()
