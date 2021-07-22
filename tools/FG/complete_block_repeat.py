#!/usr/bin/env python
import unittest
from HWTest import HWTest
from pyhalco_common import Enum
import pyhalco_hicann_v2
import pyhalbe as ph
import time as t
import numpy as np
#***PLOTTING
import matplotlib.pyplot as plt
import random
import pylab
import pickle as pickle
import pycalibtic
from FGTest import FGTest
from plot_fg_block import plot_fg_block

class FloatingGateTest(HWTest,FGTest):

    def write_fg(self, block, config, target_value_volt, target_value_curr):
        fgc = ph.HICANN.FGControl()
        fgc.setConfig(block, config)
        fgblock = fgc.getBlock(block)
        for i in range(0,24):
            for j in range(0,128):
                fgblock.setNeuronRaw(j, i, target_value_volt if i%2 else target_value_curr)
            fgblock.setSharedRaw(i, target_value_volt if i%2 else target_value_curr)
        print("set floating gates for {}, to ({}, {})".format(
                block, target_value_volt, target_value_curr))
        assert config == fgblock.getConfig()
        print(fgblock.getConfig().maxcycle)
        print(fgblock.getConfig().readtime)
        print(fgblock.getConfig().acceleratorstep)
        print(fgblock.getConfig().voltagewritetime)
        print(fgblock.getConfig().currentwritetime)
        print(fgblock.getConfig().pulselength)
        ph.HICANN.set_fg_values(self.h, block, fgblock)
        ph.HICANN.flush(self.h)

    def first_pass_cfg(self):
        fg_conf = ph.HICANN.FGConfig()
        #fg_conf.maxcycle = 90
        #fg_conf.currentwritetime = 1
        #fg_conf.voltagewritetime = 15
        #fg_conf.readtime = 15
        #fg_conf.acceleratestep = 9
        #fg_conf.pulselength = 9
        #fg_conf.fg_biasn = 5
        #fg_conf.fg_bias = 8
        return fg_conf

    def second_pass_cfg(self):
        config = ph.HICANN.FGConfig()
        config.maxcycle = 12;
        config.readtime = 63;
        config.acceleratorstep = 63;
        config.voltagewritetime = 5;
        config.currentwritetime = 20;
        config.pulselength = 2;
        #config.fg_biasn = 5
        #config.fg_bias = 8
        return config

    def write_fg_twice(self, block, target_value_volt, target_value_curr):
        self.write_fg(block, self.first_pass_cfg(),
                target_value_volt, target_value_curr)
        self.write_fg(block, self.second_pass_cfg(),
                target_value_volt, target_value_curr)


    def test_HICANNInit(self):
        '''Set complete FG-block to constant value, write FG several times in every loop! 
        Read out FG-block several times to get statistic! '''

        target_value_volt = 530
        target_value_curr = 490
        reprogram = False
        zero = True

        self.init_adc()

        fg_block_num = 0
        fg_block_coord = pyhalco_hicann_v2.FGBlockOnHICANN(Enum(fg_block_num))

        # start with default FGControl
        fgc = ph.HICANN.FGControl()
        # re-configure it
        #fg_conf = ph.HICANN.FGConfig()
        #fg_conf.maxcycle = 90
        #fg_conf.currentwritetime = 1
        #fg_conf.voltagewritetime = 15
        #fg_conf.readtime = 15
        #fg_conf.acceleratestep = 9
        #fg_conf.pulselength = 9
        #fg_conf.fg_biasn = 5
        #fg_conf.fg_bias = 8
        #fgc.setConfig(fg_block_coord,fg_conf)

        # Scramble blocks to zero
        if zero:
            self.write_fg_twice(fg_block_coord, 0, 0)

        if not reprogram:
            print("programming floating gates...")
            self.write_fg_twice(fg_block_coord, target_value_volt, target_value_curr)

        self.init_fg_measurement(fg_block_num)
        # measure
        m = []
        for i in range(3):
            print("Run No. "+str(i))
            # (re-) configure ADC and output MUX
            # this also pulls reset
            #self.init_fg_measurement(fg_block_num)

            # Write FG
            if reprogram:
                print("programming floating gates...")
                self.write_fg_twice(fg_block_coord, target_value_volt, target_value_curr)

            for a in range(0,24):
                for n in range(0,129):

                    calibrated_trace = self.read_fg_cell(n,a)
                    if a<2:
                        with open('results/single_traces/trace_'+str(a)+'_'+str(n)+'_'+str(i)+'.txt','w') as f:
                            for v in calibrated_trace:
                                f.write(str(v)+'\n')

                    value = np.mean(calibrated_trace)
                    try:
                        m[a][n].extend([(t.time(),value)])
                    except IndexError: 
                        try:
                            m[a].append([])
                            m[a][n].extend([(t.time(),value)])
                        except IndexError:
                            m.append([])
                            m[a].append([])
                            m[a][n].extend([(t.time(),value)])

            #t.sleep(10)

        name = './results/FG%d_HC%d_DNC%d_FPGA%s_%d' % (
                fg_block_num,self.HICANN,self.DNC,self.IP,target_value_volt)
        if not reprogram:
            name += '_norp'
        if zero:
            name += '_zero'
        name += '.pkl'

        print("Pickle to '{}'".format(name))
        with open(name, 'wb') as out:
            pickle.dump(m, out, pickle.HIGHEST_PROTOCOL)

        plot_fg_block(m,target_value_volt,self.IP,self.DNC,self.HICANN,fg_block_num,reprogram,True)
            
if __name__ == '__main__':
    HWTest.main()


# vim: ts=4:sw=4:expandtab: 
