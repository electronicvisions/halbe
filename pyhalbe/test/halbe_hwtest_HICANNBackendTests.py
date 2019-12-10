#!/usr/bin/env python
import unittest
from HWTest import HWTest
from pyhalbe import *
import time as t

import Coordinate
from Coordinate import Enum

class HICANNBackendHWTests(HWTest):
    def test_HICANNInit(self):
        pass

    def test_NeuronCurrentStimHWTest(self):
        fgc = HICANN.FGControl()
        fg_config = HICANN.FGConfig()
        for block in [Enum(0), Enum(1)]:
            block = Coordinate.FGBlockOnHICANN(block)
            HICANN.set_fg_config(self.h, block, fg_config)
            HICANN.set_fg_values(self.h, block, fgc.getBlock(block))

        # 2nd: configure the neuron
        nrn = HICANN.Neuron()
        #HICANN.Neuron other_nrn = HICANN.Neuron()

        nrn.address(HICANN.L1Address(42))
        nrn.activate_firing(True)
        nrn.enable_spl1_output(True)
        nrn.enable_fire_input(False)
        nrn.enable_aout(True)
        nrn.enable_current_input(True)

        nquad = HICANN.NeuronQuad()
        nquad[Coordinate.NeuronOnQuad(Coordinate.X(0), Coordinate.Y(0))] = nrn

        HICANN.set_denmem_quad(self.h, Coordinate.QuadOnHICANN(0), nquad)

        # NEURON CONFIG
        nrn_cfg = HICANN.NeuronConfig()
        nrn_cfg.bigcap.set(Coordinate.top)

        HICANN.set_neuron_config(self.h, nrn_cfg)

        # set ANALOG
        ac = HICANN.Analog()
        ac.set_membrane_top_even(Coordinate.AnalogOnHICANN(0))
        HICANN.set_analog(self.h, ac)

        # CURRENT STIMULUS
        fg_config = HICANN.FGConfig()
        fg_config.pulselength = 15
        for block in [Enum(0), Enum(1)]:
            block = Coordinate.FGBlockOnHICANN(block)
            HICANN.set_fg_config(self.h, block, fg_config)

        stimulus = HICANN.FGStimulus()
        stimulus.setContinuous(True)
        stimulus[:40] = [800] * 40
        stimulus[40:] = [0] * (len(stimulus) - 40)
        HICANN.set_current_stimulus(self.h, Coordinate.FGBlockOnHICANN(Coordinate.Enum(0)), stimulus)

        # READOUT PULSES!
        #configure merger tree, phase
        tree = HICANN.MergerTree(); #default settings are OK
        HICANN.set_merger_tree(self.h, tree)
        HICANN.set_phase(self.h)

        dnc_mergers = HICANN.DNCMergerLine()
        for i in range(8):
            # rcv from HICANN
            mer = Coordinate.DNCMergerOnHICANN(i)
            dnc_mergers[mer].config = HICANN.Merger.RIGHT_ONLY
            dnc_mergers[mer].slow = False
            dnc_mergers[mer].loopback = False
        HICANN.set_dnc_merger(self.h, dnc_mergers)

        # DNC and dnc_if:
        gbit = Coordinate.GbitLinkOnHICANN(0)
        gl = HICANN.GbitLink()
        gl.dirs[gbit.value()] = HICANN.GbitLink.Direction.TO_DNC

        gr = DNC.GbitReticle()
        gr[self.h.to_HICANNOnDNC()] = gl

        HICANN.set_gbit_link(self.h, gl)
        DNC.set_hicann_directions(self.fpga, self.dnc, gr)
        HICANN.flush(self.h)

        rec_pulses = FPGA.receive(self.fpga, self.dnc, 10000); # 10 ms

        print str(rec_pulses.size()) + " packets received"
        self.assertGreater(rec_pulses.size(), 0)

        # Check for correct addrss of spikes
        for np in range(rec_pulses.size()):
            pulse = rec_pulses.get(np)
            self.assertEqual( HICANN.L1Address(42), pulse.getNeuronAddress())
            self.assertEqual(gbit, pulse.getChannel())
            self.assertEqual(self.hicann, pulse.getChipAddress())
            self.assertEqual(self.dnc, pulse.getDncAddress())

        #turn off links
        gl.dirs[0] = HICANN.GbitLink.Direction.OFF
        gr[Coordinate.HICANNOnDNC(Coordinate.Enum(0))] = gl
        HICANN.set_gbit_link(self.h, gl)
        DNC.set_hicann_directions(self.fpga, self.dnc, gr)
        HICANN.flush(self.h)

    def testRegularPattern(self):
        pattern1 = [0]*64
        pattern2 = [1]*64

        d = [True, False, False, False]

        for i in range(0, 64):
            pattern1[i] = HICANN.CrossbarRow(d[-i%4:]+d[:-i%4])
            HICANN.set_crossbar_switch_row(self.h, Coordinate.HLineOnHICANN(i), Coordinate.left, pattern1[i])
            pattern2[i] = HICANN.get_crossbar_switch_row(self.h, Coordinate.HLineOnHICANN(i), Coordinate.left)

        self.assertEqual(pattern1, pattern2)

        for i in range(0, 64):
            pattern1[i] = HICANN.CrossbarRow(d[-i%4:]+d[:-i%4])
            HICANN.set_crossbar_switch_row(self.h, Coordinate.HLineOnHICANN(i), Coordinate.right, pattern1[i])
            pattern2[i] = HICANN.get_crossbar_switch_row(self.h, Coordinate.HLineOnHICANN(i), Coordinate.right)

        self.assertEqual(pattern1, pattern2)

    def testSyndriverRams(self):
        # syndriver switch row
        pattern1 = [HICANN.SynapseSwitchRow() for ii in range(224)]
        pattern2 = [HICANN.SynapseSwitchRow() for ii in range(224)]
        d = [True] + [False]*15

        for i in range(0, 224):
            pattern1[i][:] = d[-i%len(d):]+d[:-i%len(d)]
            side = Coordinate.left
            row = Coordinate.SynapseSwitchRowOnHICANN(Coordinate.Y(i), side)
            HICANN.set_syndriver_switch_row(self.h, row, pattern1[i])
            pattern2[i] = HICANN.get_syndriver_switch_row(self.h, row)

        self.assertEqual(pattern1, pattern2)

    def testCrossbarRamWithRegularPattern(self):
        pattern1 = [0]*64
        pattern2 = [1]*64

        d = [True, False, False, False]

        for i in range(0, 64):
                pattern1[i] = HICANN.CrossbarRow(d[-i%4:]+d[:-i%4])
                HICANN.set_crossbar_switch_row(self.h, Coordinate.HLineOnHICANN(i), Coordinate.left, pattern1[i])
                pattern2[i] = HICANN.get_crossbar_switch_row(self.h, Coordinate.HLineOnHICANN(i), Coordinate.left)

        self.assertEqual(pattern1, pattern2)

        for i in range(0, 64):
                pattern1[i] = HICANN.CrossbarRow(d[-i%4:]+d[:-i%4])
                HICANN.set_crossbar_switch_row(self.h, Coordinate.HLineOnHICANN(i), Coordinate.right, pattern1[i])
                pattern2[i] = HICANN.get_crossbar_switch_row(self.h, Coordinate.HLineOnHICANN(i), Coordinate.right)

        self.assertEqual(pattern1, pattern2)

    def test_FPGABEGTest(self):
        #configure DNC mergers
        mergers = HICANN.DNCMergerLine()
        for i in range(8):
            j = Coordinate.DNCMergerOnHICANN(i)
            if (i%2): mergers[j].config = HICANN.Merger.RIGHT_ONLY
            else: mergers[j].config = HICANN.Merger.LEFT_ONLY
            mergers[j].slow = False
            mergers[j].loopback = not(i%2)
        HICANN.set_dnc_merger(self.h, mergers)

        #configure DNC<->HICANN links
        gbit = DNC.GbitReticle()
        link = HICANN.GbitLink()
        for i in range(8):
            if (i%2): link.dirs[i] = HICANN.GbitLink.Direction.TO_DNC
            else: link.dirs[i] = HICANN.GbitLink.Direction.TO_HICANN

        gbit[self.h.to_HICANNOnDNC()] = link
        HICANN.set_gbit_link(self.h, link)
        DNC.set_hicann_directions(self.fpga, self.dnc, gbit)

        #configure FPGA Background Generator
        bg = FPGA.BackgroundGenerator()
        bg.enable        = True
        bg.poisson       = False
        bg.seed          = 12345 #cannot be zero
        bg.rate          = 300   #in DNC-clock cycles
        bg.first_address = 2
        bg.last_address  = 3 #last address is greater or equal than first
        bg.hicann_number = self.h.coordinate().onDNC()
        bg.channels[0]   = True #set active channels
        bg.channels[2]   = True
        bg.channels[4]   = True
        bg.channels[6]   = True
        FPGA.set_fpga_background_generator(self.fpga, self.dnc, bg)
        HICANN.flush(self.h)

        #generate data container and receive data
        data = FPGA.PulseEventContainer()
        data.clear()
        data = FPGA.receive(self.fpga, self.dnc, 50000) #0,05 seconds

        print str(data.size()) + " packets received"
        self.assertNotEqual(0, data.size())

        number_count = 0
        channel_count = 0
        numbers = []
        channels = []
        for i in range(data.size()):
            numbers.append(int(data[i].getNeuronAddress()))
            channels.append(data[i].getChannel().value())
        for j in range(64):
            if (numbers.count(j)): number_count += 1
        for k in range(8):
            if (channels.count(k)): channel_count += 1

        #check if number of different neuron numbers matches
        self.assertEqual(bg.last_address - bg.first_address + 1, number_count)

        #check if number of used channels matches
        self.assertEqual(bg.channels.count(), channel_count)

        #turn the BEG off
        bg.enable = False
        FPGA.set_fpga_background_generator(self.fpga, self.dnc, bg)

        #turn off the links to prevent packet collision
        for hicann in Coordinate.iter_all(Coordinate.HICANNOnDNC):
            link.dirs[hicann.toEnum().value()] = HICANN.GbitLink.Direction.OFF
            gbit[hicann] = link
        HICANN.set_gbit_link(self.h, link)
        DNC.set_hicann_directions(self.fpga, self.dnc, gbit)
        HICANN.flush(self.h)

    def test_FIFO_loopback(self):
        loop = False
        mergers = HICANN.DNCMergerLine()
        mer = HICANN.DNCMerger()
        for j in range(8):
            if (j%2): mer.config = HICANN.Merger.RIGHT_ONLY
            else: mer.config = HICANN.Merger.LEFT_ONLY
            mer.slow = False
            mer.loopback = not(j%2)
            mergers[Coordinate.DNCMergerOnHICANN(j)] = mer

        HICANN.set_dnc_merger(self.h, mergers)

        gbit = DNC.GbitReticle()
        link = HICANN.GbitLink()
        for i in range(8):
            if (i%2): link.dirs[i] = HICANN.GbitLink.Direction.TO_DNC
            else: link.dirs[i] = HICANN.GbitLink.Direction.TO_HICANN

        gbit[self.h.to_HICANNOnDNC()] = link
        HICANN.set_gbit_link(self.h, link)
        DNC.set_hicann_directions(self.fpga, self.dnc, gbit)

        sent_data = []
        received_data = FPGA.PulseEventContainer()
        sent_data.clear()
        received_data.clear()
        GbitLinkOnHICANN = Coordinate.GbitLinkOnHICANN

        for i in range(500): #500 events, first spike getTime non-zero
            sent_data.append(FPGA.PulseEvent(
                self.h.to_DNCOnFPGA(),    #DNC-number (1 for vertical setup)
                self.h.to_HICANNOnDNC(),  #HICANN number 0
                GbitLinkOnHICANN(0),      #channel number 0
                HICANN.L1Address(0),      #neuron number 0
                500*i+500))               #every 500 cycles

        #send and receive events
        received_data = FPGA.send_and_receive(
            self.fpga, self.dnc, FPGA.PulseEventContainer(sent_data), loop, 100000) #0,1 seconds

        for i in range(8): link.dirs[i] = HICANN.GbitLink.Direction.OFF
        for i in range(8):
            gbit[Coordinate.HICANNOnDNC(Coordinate.Enum(i))] = link

        HICANN.set_gbit_link(self.h, link)
        DNC.set_hicann_directions(self.fpga, self.dnc, gbit)
        HICANN.flush(self.h) #flush to hardware

        print len(sent_data), " packets sent, ", received_data.size(), " received"
        self.assertEqual(len(sent_data), received_data.size())


class HICANNL1Test(HWTest):
    def test_HICANNL1BottomLeftRepeaterHWTest(self):
        count = self.L1TransmissionTest(Coordinate.RepeaterBlockOnHICANN(Coordinate.X(0), Coordinate.Y(2)))
        self.assertGreater(count, 3) #3 or more events with correct delay (out of 16) are enough

    def test_HICANNL1BottomRightRepeaterHWTest(self):
        count = self.L1TransmissionTest(Coordinate.RepeaterBlockOnHICANN(Coordinate.X(1), Coordinate.Y(2)))
        self.assertGreater(count, 3) #3 or more events with correct delay (out of 16) are enough

    def test_HICANNL1TopLeftRepeaterHWTest(self):
        count = self.L1TransmissionTest(Coordinate.RepeaterBlockOnHICANN(Coordinate.X(0), Coordinate.Y(0)))
        self.assertGreater(count, 3) #3 or more events with correct delay (out of 16) are enough

    def test_HICANNL1TopRightRepeaterHWTest(self):
        count = self.L1TransmissionTest(Coordinate.RepeaterBlockOnHICANN(Coordinate.X(1), Coordinate.Y(0)))
        self.assertGreater(count, 3) #3 or more events with correct delay (out of 16) are enough

    def test_HICANNL1CenterRightRepeaterHWTest(self):
        count = self.L1TransmissionTest(Coordinate.RepeaterBlockOnHICANN(Coordinate.X(1), Coordinate.Y(1)))
        self.assertGreater(count, 3) #3 or more events with correct delay (out of 16) are enough

    def L1TransmissionTest(self, block):
        #period between events used in testing below
        period = 150

        #set PLL frequency
        #HICANN.set_PLL_frequency(self.h, 100)

        #configure DNC mergers
        mergers = HICANN.DNCMergerLine()
        for i in range(8):
            mer = Coordinate.DNCMergerOnHICANN(i)
            mergers[mer].config = HICANN.Merger.RIGHT_ONLY
            mergers[mer].slow = True
            mergers[mer].loopback = False
        HICANN.set_dnc_merger(self.h, mergers)

        #configure merger tree, phase
        tree = HICANN.MergerTree() #default settings are OK
        phase = HICANN.phase_t(0)
        HICANN.set_merger_tree(self.h, tree)
        HICANN.set_phase(self.h, phase)

        #configure background generator
        bgarray = HICANN.BackgroundGeneratorArray()
        for i in range(8):
            bgarray[i].enable(True)
            bgarray[i].random(False)
            bgarray[i].seed(200)
            bgarray[i].period(period)
            bgarray[i].address(HICANN.L1Address(0))
        HICANN.set_background_generator(self.h, bgarray)

        #configure sending repeaters
        rep = Coordinate.RepeaterBlockOnHICANN(Coordinate.X(0), Coordinate.Y(1))
        HICANN.set_repeater_block(self.h, rep, HICANN.RepeaterBlock())
        sr = HICANN.HorizontalRepeater()
        sr.setOutput(Coordinate.right)
        for i in range(8):
            HICANN.set_repeater(self.h, Coordinate.HLineOnHICANN(6+8*i).repeater(), sr)

        #configure crossbars and synapse switches
        cb = HICANN.Crossbar()
        ss = HICANN.SynapseSwitch()
        row_cfg = HICANN.CrossbarRow()
        syn_row_cfg = HICANN.SynapseSwitchRow()

        for i in range(8):
            if block.x().value() == 0 and block.y().value() == 2:
                cb.set(Coordinate.VLineOnHICANN(28-4*i), Coordinate.HLineOnHICANN(6+8*i), True)
                row_cfg = cb.get_row(Coordinate.HLineOnHICANN(6+8*i), Coordinate.left)
                HICANN.set_crossbar_switch_row(self.h, Coordinate.HLineOnHICANN(6+8*i), Coordinate.left, row_cfg)

            elif block.x().value() == 1 and block.y().value() == 2:
                cb.set(Coordinate.VLineOnHICANN(227+4*i), Coordinate.HLineOnHICANN(6+8*i), True)
                row_cfg = cb.get_row(Coordinate.HLineOnHICANN(6+8*i), Coordinate.right)
                HICANN.set_crossbar_switch_row(self.h, Coordinate.HLineOnHICANN(6+8*i), Coordinate.right, row_cfg)

            elif block.x().value() == 0 and block.y().value() == 0:
                addr = Coordinate.SynapseSwitchRowOnHICANN(Coordinate.Y(15-2*i), Coordinate.left)
                cb.set(Coordinate.VLineOnHICANN(28-4*i), Coordinate.HLineOnHICANN(6+8*i), True)
                ss.set(Coordinate.VLineOnHICANN(28-4*i), addr.line(), True) #connecting two parallel vertical lanes
                ss.set(Coordinate.VLineOnHICANN(29-4*i), addr.line(), True)
                row_cfg = cb.get_row(Coordinate.HLineOnHICANN(6+8*i), Coordinate.left)
                HICANN.set_crossbar_switch_row(self.h, Coordinate.HLineOnHICANN(6+8*i), Coordinate.left, row_cfg)
                syn_row_cfg = ss.get_row(addr)
                HICANN.set_syndriver_switch_row(self.h, addr, syn_row_cfg)

            elif block.x().value() == 1 and block.y().value() == 0:
                addr = Coordinate.SynapseSwitchRowOnHICANN(Coordinate.Y(15-2*i), Coordinate.right)
                cb.set(Coordinate.VLineOnHICANN(227+4*i), Coordinate.HLineOnHICANN(6+8*i), True)
                ss.set(Coordinate.VLineOnHICANN(227+4*i), addr.line(), True) #connecting two parallel vertical lanes
                ss.set(Coordinate.VLineOnHICANN(226+4*i), addr.line(), True)
                row_cfg = cb.get_row(Coordinate.HLineOnHICANN(6+8*i), Coordinate.right)
                HICANN.set_crossbar_switch_row(self.h, Coordinate.HLineOnHICANN(6+8*i), Coordinate.right, row_cfg)
                syn_row_cfg = ss.get_row(addr)
                HICANN.set_syndriver_switch_row(self.h, addr, syn_row_cfg)

            elif block.x().value() == 1 and block.y().value() == 1:
                cb.set(Coordinate.VLineOnHICANN(28-4*i), Coordinate.HLineOnHICANN(6+8*i), True)
                cb.set(Coordinate.VLineOnHICANN(28-4*i), Coordinate.HLineOnHICANN(7+8*i), True)
                row_cfg = cb.get_row(Coordinate.HLineOnHICANN(6+8*i), Coordinate.left)
                HICANN.set_crossbar_switch_row(self.h, Coordinate.HLineOnHICANN(6+8*i), Coordinate.left, row_cfg)
                row_cfg = cb.get_row(Coordinate.HLineOnHICANN(7+8*i), Coordinate.left)
                HICANN.set_crossbar_switch_row(self.h, Coordinate.HLineOnHICANN(7+8*i), Coordinate.left, row_cfg)

        #readout-routine
        vr = HICANN.VerticalRepeater()
        rb = HICANN.RepeaterBlock()
        startrep = 0 #starting readout-repeater
        registertype = HICANN.RepeaterBlock.EVEN #which readout-circuit is responsible
        dir = Coordinate.top #transmit-direction

        if block.x().value() == 0 and block.y().value() == 2:
            startrep = 0
            registertype = HICANN.RepeaterBlock.EVEN
            dir = Coordinate.bottom
        elif block.x().value() == 1 and block.y().value() == 2:
            startrep = 227
            registertype = HICANN.RepeaterBlock.EVEN
            dir = Coordinate.bottom
        elif block.x().value() == 0 and block.y().value() == 0:
            startrep = 1
            registertype = HICANN.RepeaterBlock.ODD
            dir = Coordinate.top
        elif block.x().value() == 1 and block.y().value() == 0:
            startrep = 226
            registertype = HICANN.RepeaterBlock.ODD
            dir = Coordinate.top

        success_counter = 0

        # unfortunately center right readout routine is too different...
        if not (block.x().value() == 1 and block.y().value() == 1):
            for i in range(startrep, startrep + 32, 4):
                #configure receiving repeater
                vr.setInput(dir)
                HICANN.set_repeater(self.h, Coordinate.VLineOnHICANN(i).repeater(), vr)
                HICANN.flush(self.h)

                t.sleep(0.0005) #time for the dll to lock

                #configure receiving repeater block
                rb.start_tdi[registertype] = False #reset the full flag
                HICANN.set_repeater_block(self.h, block, rb)
                rb.start_tdi[registertype] = True #start recording received data
                HICANN.set_repeater_block(self.h, block, rb)
                HICANN.flush(self.h)

                t.sleep(0.001) #recording lasts for ca. 4 us - 1 ms

                rb = HICANN.get_repeater_block(self.h, block)
                test = rb.tdi_data[registertype]

                for x in range(2): #see how many events come with correct delay (+/- 2)
                    if((abs( abs(test[x+1].time - test[x].time) - period )) < 3):
                        success_counter+=1

                #~ print "From repeater " + str(i) + " received: "
                #~ if rb.full_flag[registertype]:
                    #~ print "Full flag is set"
                #~ else:
                    #~ print "Full flag is NOT set"
                #~ print "Neuron number " + str(test[0].address) + " at time " + str(test[0].time)
                #~ print "Neuron number " + str(test[1].address) + " at time " + str(test[1].time)
                #~ print "Neuron number " + str(test[2].address) + " at time " + str(test[2].time) + "\n"

                rb.start_tdi[registertype] = False #reset the full flag TWO (!) times
                HICANN.set_repeater_block(self.h, block, rb)
                HICANN.set_repeater_block(self.h, block, rb)

                #set repeater mode to IDLE to prevent conflicts
                vr.setIdle()
                HICANN.set_repeater(self.h, Coordinate.VLineOnHICANN(i).repeater(), vr)
        else:
            hr = HICANN.HorizontalRepeater()
            for i in range(7, 64, 8):
                #configure receiving repeater
                hr.setInput(Coordinate.right)
                HICANN.set_repeater(self.h, Coordinate.HLineOnHICANN(i).repeater(), hr)
                HICANN.flush(self.h)

                t.sleep(0.0005) #time for the dll to lock

                #configure receiving repeater block
                rb.start_tdi[HICANN.RepeaterBlock.EVEN] = False #reset the full flag
                HICANN.set_repeater_block(self.h, block, rb)
                rb.start_tdi[HICANN.RepeaterBlock.EVEN] = True #start recording received data
                HICANN.set_repeater_block(self.h, block, rb)
                HICANN.flush(self.h)

                t.sleep(0.0001) #recording lasts for ca. 4 us - 1 ms

                rb = HICANN.get_repeater_block(self.h, block)
                test = rb.tdi_data[HICANN.RepeaterBlock.EVEN]

                #~ print "From repeater " + str(i) + " received: "
                #~ if rb.full_flag[registertype]:
                    #~ print "Full flag is set"
                #~ else:
                    #~ print "Full flag is NOT set"
                #~ print "Neuron number " + str(test[0].address) + " at time " + str(test[0].time)
                #~ print "Neuron number " + str(test[1].address) + " at time " + str(test[1].time)
                #~ print "Neuron number " + str(test[2].address) + " at time " + str(test[2].time) + "\n"

                for x in range(2): #see how many events come with correct delay
                    if((abs( abs(test[x+1].time - test[x].time) - period )) < 3):
                        success_counter+=1

                rb.start_tdi[HICANN.RepeaterBlock.EVEN] = False #reset the full flag TWO (!) times
                HICANN.set_repeater_block(self.h, block, rb)
                HICANN.set_repeater_block(self.h, block, rb)

                #set repeater mode to IDLE to prevent conflicts
                hr.setIdle()
                HICANN.set_repeater(self.h, Coordinate.HLineOnHICANN(i).repeater(), hr)
        HICANN.flush(self.h)
        return success_counter

if __name__ == '__main__':
    HICANNBackendHWTests.main()
