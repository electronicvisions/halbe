import time
import matplotlib.pyplot as plt
import numpy as np
import pylab
from HWTest import HWTest
import pyhalbe as ph
import pycalibtic
from FGTest import FGTest
import cPickle

class FloatingGateBias(HWTest,FGTest):
    '''Write a single row (as determined by self.row_coord) to maximum
       DAC value and program it down with small single pulses of the
       floating gate controller. Measure new voltage afterwards and pickle
       the results to file FG_characterize.txt'''


    def meassure_bias(self, fg_value):
        self.init_fg_measurement(0) # FGBlock 0
        fg_conf_fast = ph.HICANN.FGConfig()
        fg_conf_fast.voltagewritetime = 63
        fg_conf_fast.currentwritetime = 63
        fg_conf_fast.maxcycle = 8
        fg_conf_fast.pulselength = 12
        fg_conf_fast.readtime = 30
        fg_conf_fast.acceleratorstep = 63
        fg_conf_fast.fg_bias = 0
        fg_conf_fast.fg_biasn = 3

        print "Clear floating_gates"
        # instantiate FG stuff
        fgc = ph.HICANN.FGControl()
        fgb = fgc.getBlock(self.fg_coord)
        # program row up fast
        fgb.setConfig(fg_conf_fast)
        self.set_block(fgb, 0)
        ph.HICANN.set_fg_values(self.h, self.fg_coord, fgb)

        fg_conf = ph.HICANN.FGConfig()
        fg_conf.readtime = 63
        fgb.setConfig(fg_conf)

        ph.HICANN.flush(self.h)
        self.set_block(fgb, fg_value)
        ph.HICANN.set_fg_values(self.h, self.fg_coord, fgb)
        ph.HICANN.set_fg_values(self.h, self.fg_coord, fgb)
        ph.HICANN.flush(self.h)
        print np.mean(self.read_fg_cell(1,int(self.row_coord)))

        data = []
        for bias in (0, 1, 2, 3, 4, 8, 12, 15):
            print "Meassure for bias {}".format(bias)
            cfg = ph.HICANN.FGConfig()
            cfg.fg_biasn = bias
            ph.HICANN.set_fg_config(self.h, self.fg_coord, cfg)

            cells = []
            for cell in range(129):
                calibrated_trace = self.read_fg_cell(cell, int(self.row_coord))
                cells.append(np.mean(calibrated_trace))
            data.append((bias, np.array(cells)))
        return data

    def test_HICANNInit(self):
        self.init_adc()
        row_number = 0
        repetions = 5
        self.row_coord = ph.Coordinate.FGRowOnFGBlock(row_number)

        filename = 'results/bias_characterize_%s_%d_%d_%d.txt'%(
                self.IP, self.DNC, self.HICANN, row_number)

        result = []
        for fg_value in np.linspace(0, 1024, 20):
            fg_value = int(fg_value)
            for rep in range(repetions):
                for bias, data in self.meassure_bias(fg_value):
                    result.append((fg_value, bias, rep, data))

            with open(filename, 'wb') as output:
                cPickle.dump(result, output, cPickle.HIGHEST_PROTOCOL)

if __name__ == '__main__':
    HWTest.main()

# vim: ts=4:sw=4:expandtab: 

