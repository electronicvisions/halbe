#!/usr/bin/env python
import unittest
import time as t
import numpy as np
from HWTest import HWTest
from pyhalbe import *
from HWTestLib import *
from matplotlib import pylab

class HWTestLibExamples(HWTest):
    def test_ProgramFloatingGates(self):
        fgc = LoadDefaultFGParameters()
        fgc = HICANN.FGControl()
        ProgramFG(self.h, fgc)

    def test_RecordActivationCurve(self):
        fgc = LoadDefaultFGParameters()
        period = 2000
        neuron = 10
        RecordActivationCurve(self.h, 0, 100, period, neuron, False, fgc, neuron)

    def test_ShowMembraneConstCurrent(self):
        neuron = 0
        StimulateNeuronWithCurrent(self.h, neuron, 500)
        PutNeuronToAout(self.h, neuron, 1, True) # implicit neuron activation here!

        (voltage, time) = GetADCTrace(100, 1)
        pylab.plot(time, voltage)
        pylab.show()

    def test_ShowMembraneStepCurrent(self):
        neuron = 0
        StimulateNeuronWithStepCurrent(self.h, neuron, 100, 30)
        PutNeuronToAout(self.h, neuron, 1, True) # implicit neuron activation here!

        (voltage, time) = GetADCTrace(100, 1)
        pylab.plot(time, voltage)
        pylab.show()

    def test_ShowMembraneSynapseStim(self):
        neuron = 1
        StimulateNeuronWithSynapses(self.h, neuron, False, 2000, True, False)
        PutNeuronToAout(self.h, neuron, 1, False) # implicit neuron activation here!

        t.sleep(0.001)
        (voltage, time) = GetADCTrace(100, 1)
        pylab.plot(time, voltage)
        pylab.show()

    def test_MembraneTimeConstants(self):
        for i in range(1):
            tc = GetMembraneTimeConstant(self.h, i)
            print "Neuron " + str(i) + " has a membrane time constant of " + str(tc) + " microseconds"

    def test_SynapticTimeConstants(self):
        for i in range(1):
            tc = GetSynapticTimeConstant(self.h, i, False)
            print "Neuron " + str(i) + " has a synaptic time constant of " + str(tc) + " microseconds"

    def test_RestPotential(self):
        for i in range(1):
            p = GetMembraneRestPotential(self.h, i)
            print "Neuron " + str(i) + " has rest potential at " + str(p) + " Volts"

    def test_ResetPotential(self):
        fgc = LoadResetFGParameters()
        ProgramFG(self.h, fgc)
        for i in range(1):
            r = GetResetPotential(self.h, i)
            print "Neuron " + str(i) + " has reset potential at " + str(r) + " Volts"

    def test_Threshold(self):
        for i in range(1):
            th = GetFiringThreshold(self.h, i)
            print "Neuron " + str(i) + " has firing threshold at " + str(th) + " Volts"

    def test_RefractoryPeriod(self):
        fgc = LoadResetFGParameters()
        ProgramFG(self.h, fgc)
        for i in range(1):
            r = GetRefractoryPeriod(self.h, i)
            print "Neuron " + str(i) + " has refractory period of " + str(r) + " microseconds"

    def test_LinearityOfSynapticInput(self):
        for n in range(1):
            ints = TakeLinearityCurve(self.h, n, False, 3, 1000)
            pylab.plot(range(1,16), ints[1:])
            pylab.show()

    def test_FitPSP(self):
        for i in range (1):
            (psp, time) = GetAveragePSP(self.h, i, 15, False, 2000, True)
            from fit import fit_psp_simple, ideal_psp
            fitpar = fit_psp_simple(time, psp)
            pylab.plot(time, psp)
            pylab.plot(time, ideal_psp(fitpar, pylab.array(time)))
            pylab.show()

    def L1TransmissionTest(self, block, hicann):
        #period between events used in testing below
        period = 150

        #prepare HICANN for an experiment, executes a full reset
        #SetL1Voltages(hicann, 0.8, 1.0) #set voltages
        #HICANN.reset(hicann, 100) #set PLL here
        #HICANN.init(hicann, False) #initialize highspeed

        #configure merger tree, DNC-mergers, phase
        SetMergerTreeDefaults(hicann)

        #configure background generator
        StartAllBEG(hicann, period, 0, False)

        #configure sending repeaters
        SetSendingRepeaterDefaults(hicann)

        #configure crossbars and synapse switches
        for i in range(8):
            if block.toEnum().value() == 4:
                SetLeftCrossbarLine(hicann, 28-4*i, 6+8*i)

            elif block.toEnum().value() == 5:
                SetRightCrossbarLine(hicann, 227+4*i, 6+8*i)

            elif block.toEnum().value() == 0:
                SetLeftCrossbarLine(hicann, 28-4*i, 6+8*i)
                ss = HICANN.SynapseSwitch()
                addr = Coordinate.SynapseSwitchRowOnHICANN(Coordinate.Y(15-2*i), Coordinate.left)
                ss.set(Coordinate.VLineOnHICANN(28-4*i), addr.line(), True) #connecting two parallel vertical lanes
                ss.set(Coordinate.VLineOnHICANN(29-4*i), addr.line(), True)
                syn_row_cfg = ss.get_row(addr)
                HICANN.set_syndriver_switch_row(hicann, addr, syn_row_cfg)

            elif block.toEnum().value() == 1:
                SetRightCrossbarLine(hicann, 227+4*i, 6+8*i)
                ss = HICANN.SynapseSwitch()
                addr = Coordinate.SynapseSwitchRowOnHICANN(Coordinate.Y(15-2*i), Coordinate.right)
                ss.set(Coordinate.VLineOnHICANN(227+4*i), addr.line(), True) #connecting two parallel vertical lanes
                ss.set(Coordinate.VLineOnHICANN(226+4*i), addr.line(), True)
                syn_row_cfg = ss.get_row(addr)
                HICANN.set_syndriver_switch_row(hicann, addr, syn_row_cfg)

        #readout-routine
        startrep = 0 #starting readout-repeater
        if   block.toEnum().value() == 4: startrep = 0   #bottom left
        elif block.toEnum().value() == 5: startrep = 227 #bottom right
        elif block.toEnum().value() == 0: startrep = 1   #top left
        elif block.toEnum().value() == 1: startrep = 226 #top right

        success_counter = 0
        for i in range(startrep, startrep + 32, 4):
            times, addresses = L1Receive(hicann, block.toEnum().value(), i)

            for x in range(2): #see how many events come with correct delay (+/- 2)
                if((abs( abs(times[x+1] - times[x]) - period )) < 3):
                    success_counter+=1

            #print "From repeater " + str(i) + " received: "
            #print "Neuron number " + str(addresses[0]) + " at time " + str(times[0])
            #print "Neuron number " + str(addresses[1]) + " at time " + str(times[1])
            #print "Neuron number " + str(addresses[2]) + " at time " + str(times[2]) + "\n"

        return success_counter

    def test_HICANNL1RepeaterHWTest(self):
        count = self.L1TransmissionTest(Coordinate.RepeaterBlockOnHICANN(Coordinate.X(0), Coordinate.Y(2)), self.h)
        count += self.L1TransmissionTest(Coordinate.RepeaterBlockOnHICANN(Coordinate.X(1), Coordinate.Y(2)), self.h)
        count += self.L1TransmissionTest(Coordinate.RepeaterBlockOnHICANN(Coordinate.X(0), Coordinate.Y(0)), self.h)
        count += self.L1TransmissionTest(Coordinate.RepeaterBlockOnHICANN(Coordinate.X(1), Coordinate.Y(0)), self.h)
        self.assertGreater(count, 6) #3 or more events with correct delay (out of 16) are enough

    #~ def test_MultiHICANN_FPGABEGTest(self):
        #~ testaddr   = 20
        #~ senderline = 62
        #~ receiver   = 0 #0-7
        #~ lockperiod = 3000
        #~ mainperiod = 100
        #~ frequency  = 100
        #~ vol = 0.7
        #~ voh = 0.9
#~ 
        #~ #SetL1Voltages(self.h, vol, voh) #set voltages
        #~ #HICANN.reset(self.h, frequency) #set PLL here
        #~ #HICANN.init(self.h, False) #initialize highspeed
#~ 
        #~ #repblock, repnr = SetL1Crossbar(self.h1, senderline, receiver)
#~ 
        #~ L1SendAll(self.h2, mainperiod)
#~ 
        #~ for i in [0,8,16,24,32,40,48,56]:
            #~ #times, addresses = L1Receive(self.h3, repblock, repnr)
            #~ #times, addresses = L1Receive(self.h3, repblock, repnr)
            #~ #times, addresses = L1ReceiveVer(self.h0, repblock, repnr+2)
            #~ #times, addresses = L1ReceiveVer(self.h0, repblock, repnr+2)
            #~ #times, addresses = L1ReceiveVer(self.h1, repblock, repnr+2)
            #~ #times, addresses = L1ReceiveVer(self.h1, repblock, repnr+2)
            #~ #times, addresses = L1ReceiveVer(self.h2, repblock, repnr+2)
            #~ #times, addresses = L1ReceiveVer(self.h2, repblock, repnr+2)
            #~ #times, addresses = L1ReceiveVer(self.h3, repblock, repnr+2)
            #~ #times, addresses = L1ReceiveVer(self.h3, repblock, repnr+2)
            #~ #times, addresses = L1ReceiveVer(self.h4, repblock, repnr+2)
            #~ #times, addresses = L1ReceiveVer(self.h4, repblock, repnr+2)
            #~ #times, addresses = L1ReceiveVer(self.h5, repblock, repnr+2)
            #~ #times, addresses = L1ReceiveVer(self.h5, repblock, repnr+2)
            #~ #times, addresses = L1ReceiveVer(self.h6, repblock, repnr+2)
            #~ #times, addresses = L1ReceiveVer(self.h6, repblock, repnr+2)
            #~ #times, addresses = L1ReceiveVer(self.h7, repblock, repnr+2)
            #~ #times, addresses = L1ReceiveVer(self.h7, repblock, repnr+2)
            #~ times, addresses = L1ReceiveHor(self.h3, i)
            #~ times, addresses = L1ReceiveHor(self.h3, i)
            #~ print addresses
#~ 
        #~ #stop FPGA BEG
        #~ #StopFPGABEG(self.h2, self.dnc, self.fpga)

    def test_FPGABEGTest(self):
        testaddr   = 21
        senderline = 62
        receiver   = 0 #0-7
        mainperiod = 100

        repblock, repnr = SetL1Crossbar(self.h, senderline, receiver)
        L1Send(self.h, self.fpga.dnc(self.dnc), self.fpga, mainperiod, senderline, testaddr)

        for i in range(15):
            times, addresses = L1Receive(self.h, repblock, repnr)
            print "From repeater " + str(repnr) + " received: "
            print "Neuron number " + str(addresses[0]) + " at time " + str(times[0])
            print "Neuron number " + str(addresses[1]) + " at time " + str(times[1])
            print "Neuron number " + str(addresses[2]) + " at time " + str(times[2]) + "\n"
            

        StopFPGABEG(self.h, self.fpga.dnc(self.dnc), self.fpga)

if __name__ == '__main__':
    HWTestLibExamples.main()
