import pyhalbe as ph
import pycalibtic
import pylab
from pyhalco_common import Enum, X, Y
import pyhalco_hicann_v2

class FGTest:
    adcs = {
        '192.168.1.3': {1: ("23", 0, 1)},
        '192.168.1.1': {0: ("03", 0, 1)},
        '192.168.1.17': {1: ("13", 0, 1)},
    }

    def init_adc_calibration_backend(self, adc_id):
        '''Initialize the backend which contains ADC calibration data.
        The data will be used to convert raw ADC values to voltages.'''
        backend = pycalibtic.loadBackend(
            pycalibtic.loadLibrary("libcalibtic_xml.so"))
        backend.init()
        self.adc_calib = pycalibtic.ADCCalibration()
        self.adc_calib.load(backend, adc_id)

    def set_block_row(self,fgb,row,value):
        for j in range(0,128):
            fgb.setNeuronRaw(j, int(row), value)
        fgb.setSharedRaw(int(row), value)

    def set_block(self,fgb,value):
        for i in range(24):
            self.set_block_row(fgb,i,value)

    def set_block_pattern(self, fgb, pattern):
        assert pattern.shape == (24, 129)
        for row in range(24):
            fgb.setSharedRaw(row, pattern[row][0])
            for cell in range(128):
                fgb.setNeuronRaw(cell, row, pattern[row][cell])

    def read_fg_cell(self,cell_x,cell_y):
        ph.HICANN.set_fg_cell(self.h, self.fg_coord, pyhalco_hicann_v2.FGCellOnFGBlock(X(cell_x),Y(cell_y)))
        ph.HICANN.flush(self.h)
        ph.ADC.trigger_now(self.adc)
        raw_trace = ph.ADC.get_trace(self.adc)
        trace = pylab.array(raw_trace, dtype=pylab.ushort)
        calibrated_trace = self.adc_calib.apply(self.adc_channel, trace) 
        return calibrated_trace

    def get_adc_coords(self, analog=Coordinate.AnalogOnHICANN(0)):
        adc, adc_channel0, adc_channel1 = self.adcs[self.IP][self.DNC]
        adc_channel = (adc_channel0, adc_channel1)[analog.value()]
        return ph.ADC.USBSerial(adc), Coordinate.ChannelOnADC(adc_channel)

    def init_adc(self):

        adc,adc_channel = self.get_adc_coords()	
        self.adc_channel = pyhalco_hicann_v2.ChannelOnADC(adc_channel)
        self.adc_board = pyhalco_hicann_v2.ADC(adc)

        print "Trying to get ADC Handle."
        self.adc = ph.Handle.ADCHw(self.adc_board)

        #configure ADC
        adc_id = ph.ADC.get_board_id(self.adc)
        self.init_adc_calibration_backend(adc_id)
        trig  = pyhalco_hicann_v2.TriggerOnADC(0)
        adc_conf  = ph.ADC.Config(5000, self.adc_channel, trig)
        ph.ADC.config(self.adc, adc_conf)

        print "ADC set up."

    def init_fg_measurement(self,block):
        if isinstance(block, pyhalco_hicann_v2.FGBlockOnHICANN):
            self.fg_coord = block
        else:
            self.fg_coord = pyhalco_hicann_v2.FGBlockOnHICANN(Enum(block))

        print "Trying FPGA reset."
        ph.FPGA.reset(self.fpga)
        print "Trying HICANN init."
        ph.HICANN.init(self.h, False)

        print "Reset and init finished."

        # set ANALOG
        ac = ph.HICANN.Analog()
        ac.set_fg_left(pyhalco_hicann_v2.AnalogOnHICANN(0))
        ac.set_fg_left(pyhalco_hicann_v2.AnalogOnHICANN(1))
        ph.HICANN.set_analog(self.h, ac)

        print "Analog output configured."


