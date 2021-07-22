import time
import matplotlib.pyplot as plt
import numpy as np
import pylab
from HWTest import HWTest
import pyhalbe as ph
import pycalibtic
from FGTest import FGTest

class FloatingGateCharacterization(HWTest,FGTest):
    '''Write a single row (as determined by self.row_coord) to maximum
       DAC value and program it down with small single pulses of the
       floating gate controller. Measure new voltage afterwards and pickle
       the results to file FG_characterize.txt'''


    def test_HICANNInit(self):

        self.init_adc()
        self.init_fg_measurement(0) # FGBlock 0

        row_number = 2
        self.row_coord = ph.Coordinate.FGRowOnFGBlock(row_number)

        program_dwn = True
        program_up = True

        # instantiate FG stuff
        fgc = ph.HICANN.FGControl()
        fgb = fgc.getBlock(self.fg_coord)

        # FG config
        fg_conf_fast = ph.HICANN.FGConfig()
        #fg_conf_fast.voltagewritetime = 63
        #fg_conf_fast.currentwritetime = 63
        #fg_conf_fast.maxcycle = 255
        #fg_conf_fast.pulselength = 15
        #fg_conf_fast.readtime = 63
        #fg_conf_fast.acceleratorstep = 1

        # FG config
        fg_conf_slow_up = ph.HICANN.FGConfig()
        fg_conf_slow_up.voltagewritetime = 63
        fg_conf_slow_up.currentwritetime = 63
        fg_conf_slow_up.maxcycle = 10
        fg_conf_slow_up.pulselength = 12
        fg_conf_slow_up.readtime = 1
        fg_conf_slow_up.acceleratorstep = 63
        fg_conf_slow_up.fg_bias = 0
        fg_conf_slow_up.fg_biasn = 3

        fg_conf_slow_down = fg_conf_slow_up

        # FG config
        # fg_conf_slow_down = ph.HICANN.FGConfig()
        # fg_conf_slow_down.voltagewritetime = 20
        # fg_conf_slow_down.currentwritetime = 2
        # fg_conf_slow_down.maxcycle = 1
        # fg_conf_slow_down.pulselength = 0
        # fg_conf_slow_down.readtime = 30
        # fg_conf_slow_down.acceleratorstep = 35
        # fg_conf_slow_down.fg_bias = 7
        # fg_conf_slow_down.fg_biasn = 7

        num_runs = 10

        cells = {'config':{
            'row': str(self.row_coord),
            'ip': self.IP,
            'dnc': str(self.DNC),
            'hicann': str(self.HICANN),
            'block': str(self.fg_coord),
            'up' : str(fg_conf_slow_up),
            'down' : str(fg_conf_slow_down),
            'num_runs':num_runs
        },'data':np.zeros(shape=(33,2*num_runs))}

        if program_dwn:
            # program row up fast
            fgb.setConfig(fg_conf_fast)
            self.set_block(fgb,1023)

            print("Programming FGs completely up.")
            ph.HICANN.set_fg_values(self.h,self.fg_coord,fgb)
            ph.HICANN.set_fg_values(self.h,self.fg_coord,fgb)
            ph.HICANN.flush(self.h)
            print(np.mean(self.read_fg_cell(1,int(self.row_coord))))
            # set fgb to program row down slowly
            self.set_block(fgb,0)
            fgb.setConfig(fg_conf_slow_down)
            fgc.setBlock(self.fg_coord,fgb)

            print("Running",num_runs," programming steps down.")
            for run in range(num_runs):
                if not run%10:
                    print(run)
                for i in range(33):
                    calibrated_trace = self.read_fg_cell(i,int(self.row_coord))
                    cells['data'][i][run] = np.mean(calibrated_trace)

                ph.HICANN.set_fg_row_values(self.h,self.row_coord,fgc,True)
                ph.HICANN.flush(self.h)

        if program_up:
            # program row down fast
            self.set_block(fgb,0)
            fgb.setConfig(fg_conf_fast)

            print("Programming FGs completely down.")
            ph.HICANN.set_fg_values(self.h,self.fg_coord,fgb)
            ph.HICANN.set_fg_values(self.h,self.fg_coord,fgb)
            ph.HICANN.flush(self.h)
            print(np.mean(self.read_fg_cell(1,int(self.row_coord))))
            # program row up slowly
            self.set_block(fgb,1023)
            fgb.setConfig(fg_conf_slow_up)
            fgc.setBlock(self.fg_coord,fgb)

            print("Running",num_runs," programming steps up.")
            for run in range(num_runs):
                if not run%10:
                    print(run)
                for i in range(33):
                    calibrated_trace = self.read_fg_cell(i,int(self.row_coord))
                    cells['data'][i][run+num_runs] = np.mean(calibrated_trace)

                ph.HICANN.set_fg_row_values(self.h,self.row_coord,fgc,False)
                ph.HICANN.flush(self.h)

        import pickle
        output = open('results/FG_characterize_%s_%d_%d_%d.txt'%(self.IP,self.DNC,self.HICANN,row_number),'wb')
        pickle.dump(cells,output,pickle.HIGHEST_PROTOCOL)
        output.close()


if __name__ == '__main__':
    HWTest.main()

# vim: ts=4:sw=4:expandtab: 

