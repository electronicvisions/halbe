#!/usr/env python
# -*- coding: utf-8 -*-
# pylint: disable=missing-docstring

import time
import pycalibtic
import pyhalco_hicann_v2 as Coordinate
from pyhalbe import ADC, Handle

class ADCReadout(object):
    def __init__(self, adc_coord, readtime,
                 adc_channel=Coordinate.ChannelOnADC(0)):
        self.adc_coord = adc_coord
        self.adc_channel = adc_channel
        self.adc_readtime = readtime
        self.calibration = self.init_adc_calibration_backend(adc_coord)
        self.handle = None

    @staticmethod
    def init_adc_calibration_backend(adc_id):
        '''Initialize the backend which contains ADC calibration data.
        The data will be used to convert raw ADC values to voltages.'''
        library = pycalibtic.loadLibrary("libcalibtic_xml.so")
        backend = pycalibtic.loadBackend(library)
        backend.init()
        adc_calib = pycalibtic.ADCCalibration()
        adc_calib.load(backend, adc_id)
        return adc_calib

    def switch_channel(self, adc_channel=None):
        if self.handle is None:
            raise RuntimeError("Use only in with statement: "
                               "with ADCReadout(..) as adc:")
        if adc_channel is not None:
            self.adc_channel = adc_channel
        trigger = Coordinate.TriggerOnADC(0)
        adc_conf = ADC.Config(5000, self.adc_channel, trigger)
        ADC.config(self.handle, adc_conf)

    def read_trace(self):
        if self.handle is None:
            raise RuntimeError("Use only in with statement: "
                               "with ADCReadout(..) as adc:")
        ADC.trigger_now(self.handle)
        time.sleep(self.adc_readtime / 96e6)
        trace = ADC.get_trace(self.handle)
        return self.calibration.apply(self.adc_channel, trace)

    def __enter__(self):
        self.handle = Handle.ADCHw(self.adc_coord)
        self.switch_channel()
        return self

    def __exit__(self, error_type, value, traceback):
        Handle.freeADCHw(self.handle)
        self.handle = None
