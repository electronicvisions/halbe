import unittest
import time as t
import numpy as np
import matplotlib.pyplot as plt
import random
import plot_fg_random_row
from HWTest import HWTest
from FGTest import FGTest
import pyhalbe as ph
import pycalibtic

class FGRandomTest(HWTest,FGTest):
    '''Program each cell of one row multiple times with random
       values.'''

    def test_HICANNInit(self):#, parameters):
        afterburn = True
        row_number = 7
        fg_block_num = 0
        self.init_adc()
        self.init_fg_measurement(fg_block_num)

        fgc = ph.HICANN.FGControl()
        fgb = fgc.getBlock(self.fg_coord)

        ph.HICANN.set_fg_values(self.h, self.fg_coord, fgb)
        ph.HICANN.flush(self.h)

        Digital = []
        Analog = []
        zeitwerte = [] 

        for i in range(100):
                print "Run No. "+str(i)

                v = random.randint(0,1023)
                Digital.append(v)
                if row_number==6:
                    fgb.setShared(self.fg_coord, ph.HICANN.shared_parameter.V_dep, v)
                elif row_number==7:
                    fgb.setShared(self.fg_coord, ph.HICANN.shared_parameter.I_bstim, v)
                else:
                    raise 

                for n in range(0,128):
                    nrn = ph.Coordinate.NeuronOnFGBlock(n)
                    v = random.randint(0,1023)
                    Digital.append(v)
                    if row_number==6:
                        fgb.setNeuron(self.fg_coord, nrn, ph.HICANN.neuron_parameter.E_l, v)
                    elif row_number==7:
                        fgb.setNeuron(self.fg_coord, nrn, ph.HICANN.neuron_parameter.I_fire, v)

                fgc.setBlock(self.fg_coord,fgb)
                        
                start = t.time()
                ph.HICANN.set_fg_row_values(self.h, ph.Coordinate.FGRowOnFGBlock(row_number), fgc, True)
                ph.HICANN.flush(self.h)
                ph.HICANN.set_fg_row_values(self.h, ph.Coordinate.FGRowOnFGBlock(row_number), fgc, False)
                ph.HICANN.flush(self.h)

                if afterburn:
                    fgconf = ph.HICANN.FGConfig()
                    fgconf.voltagewritetime = 5
                    fgconf.currentwritetime = 20
                    fgconf.maxcycle = 10
                    fgconf.pulselength = 1
                    fgconf.readtime = 63
                    fgconf.acceleratorstep = 16

                    fgc.setConfig(self.fg_coord,fgconf)

                    ph.HICANN.set_fg_row_values(self.h, ph.Coordinate.FGRowOnFGBlock(row_number), fgc, True)
                    ph.HICANN.flush(self.h)
                    ph.HICANN.set_fg_row_values(self.h, ph.Coordinate.FGRowOnFGBlock(row_number), fgc, False)
                    ph.HICANN.flush(self.h)

                ende = t.time()
                zeitwerte.append(ende-start)

                for n in range(0,129):
                    calibrated_trace = self.read_fg_cell(n,row_number)

                    value = np.mean(calibrated_trace)
                    Analog.append(value)
                        
        z = np.mean(zeitwerte)
        print z, 'Sekunden'
        plot_fg_random_row.plot(Digital, Analog, z, self.IP, self.DNC, self.HICANN, fg_block_num, row_number, True)

if __name__ == '__main__':
    HWTest.main()


# vim: ts=4:sw=4:expandtab: 
