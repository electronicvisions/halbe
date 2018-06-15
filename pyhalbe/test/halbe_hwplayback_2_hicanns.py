#!/usr/bin/env python
from pyhalbe import *
import unittest
import time
import numpy as np

def flip_channel(channel):
    """
    flips GbitLinkOnHICANN as it is expectd after loopback
    """
    assert isinstance(channel, Coordinate.GbitLinkOnHICANN)
    nn = int(channel)
    return Coordinate.GbitLinkOnHICANN( int(nn/2)*2 + (not(nn%2)))

def hicann_loopback(h):
    """
    sets loopback on HICANN 0, such that channel 0->1, 2->3, etc.
    """
    assert isinstance(h, Handle.HICANN)


    # configure DNC mergers
    mergers = HICANN.DNCMergerLine()
    mer = HICANN.DNCMerger()
    for j in range(8):
        if (j%2):
            mer.config = HICANN.Merger.RIGHT_ONLY
        else:
            mer.config = HICANN.Merger.LEFT_ONLY
        mer.slow = False
        mer.loopback = not(j%2) # 0->1, 2->3 etc...
        mergers[Coordinate.DNCMergerOnHICANN(j)] = mer
    HICANN.set_dnc_merger(h, mergers)

    link = HICANN.GbitLink()
    for i in range(8):
        if (i%2):
            link.dirs[i] = HICANN.GbitLink.Direction.TO_DNC
        else:
            link.dirs[i] = HICANN.GbitLink.Direction.TO_HICANN

    HICANN.set_gbit_link(h, link)
    return link


class PlaybackTraceHWTest(unittest.TestCase):
    def setUp(self):
        #raise unittest.SkipTest("Test fail and takes no paramters: see Issue 1227")

        self.setUpCalled = False
        # ugly hack to support nose-based execution...
        self.IP = '192.168.1.5'
        self.DNC      = 1
        self.FPGA     = 0
        self.LOGLEVEL = 2
        self.ON_WAFER = False
        self.WAFER  = 0
        self.USE_SCHERIFF = False
        Enum = Coordinate.Enum
        highspeed = True
        arq = True
        hicann_num = 2
        ip = Coordinate.IPv4.from_string(self.IP)
        if self.LOGLEVEL >= 0:
            Debug.change_loglevel(self.LOGLEVEL)

        self.dnc = Coordinate.DNCOnFPGA(Enum(self.DNC))
        self.f = Coordinate.FPGAGlobal(Enum(self.FPGA), Coordinate.Wafer(Enum(self.WAFER)))

        self.fpga = Handle.createFPGAHw(self.f, ip, self.dnc, self.ON_WAFER, hicann_num)
        if self.USE_SCHERIFF:
            self.fpga.enableScheriff()
        self.addCleanup(Handle.freeFPGAHw, self.fpga)

        self.h1  = self.fpga.get(self.dnc, Coordinate.HICANNOnDNC(Enum(0)))
        if hicann_num > 1:
            self.h2  = self.fpga.get(self.dnc, Coordinate.HICANNOnDNC(Enum(4)))
        self.hicanns = [self.h1, self.h2]

        # ECM says: this should be part of the tests... automatic resets during construction isn't very "standalone"
        # OR it is necessary and in this case it should be a member function and documented somehow.
        # FPGA reset
        FPGA.reset(self.fpga)
        HICANN.init(self.h1, False)
        if hicann_num > 1:
            HICANN.init(self.h2, False)

    def runTest(self):
        ########################
        #  configure hardware for hicann loopback
        ########################
        print self.fpga, self.h1, self.dnc
        link1 = hicann_loopback(self.h1)
        link2 = hicann_loopback(self.h2)

        gbit = DNC.GbitReticle()
        gbit[self.h1.to_HICANNOnDNC()] = link1
        gbit[self.h2.to_HICANNOnDNC()] = link2

        DNC.set_hicann_directions(self.fpga, self.dnc, gbit)

        # make sure to switch off dnc loopback
        loopback = DNC.Loopback()
        DNC.set_loopback(self.fpga, self.dnc, loopback)

        ########################
        #  generate input data
        ########################
        DNCOnFPGA = Coordinate.DNCOnFPGA
        HICANNOnDNC = Coordinate.HICANNOnDNC
        GbitLinkOnHICANN = Coordinate.GbitLinkOnHICANN
        Enum = Coordinate.Enum

        #  5000 Pulses with ISI = 500 clks (2000 ns)
        pulse_events = []
        isi = 50 # every 500 cycles
        start_offset = 500
        num_pulses = 100
        hicann_num = len(self.hicanns)
        for i in range(num_pulses):
            pulse_address = FPGA.PulseAddress(
                    DNCOnFPGA(1),
                    self.hicanns[i%hicann_num].to_HICANNOnDNC(),
                    GbitLinkOnHICANN(6),
                    HICANN.Neuron.address_t(25))
            pulse_events.append(FPGA.PulseEvent(pulse_address, isi * i + start_offset))
        runtime_in_dnc_cycles = pulse_events[-1].getTime() + 200 # + offset for round trip

        ########################
        # run experiment
        ########################

        #  trigger synchronous start of time counters
        FPGA.sync_systime_counters(self.fpga)

        FPGA.write_playback_pulses(
                self.fpga,
                FPGA.PulseEventContainer(pulse_events),
                runtime_in_dnc_cycles,
                0 # fpga_hicann_delay, 63 fpga_clk cycles are 504 nano seconds
                )
        FPGA.prime_experiment(self.fpga)
        FPGA.start_experiment(self.fpga)

        received_data = FPGA.read_trace_pulses(self.fpga, runtime_in_dnc_cycles).events

        ########################
        #  compare data
        ########################

        # There should be as many
        self.assertEqual(len(pulse_events), received_data.size())

        delays = []
        # expected pulse address is the configured pulse address with dnc if channel flipped
        for i in range(received_data.size()):
            rpa = FPGA.PulseAddress(received_data[i])
            exp_pa = FPGA.PulseAddress(pulse_events[i])
            exp_pa.setChannel( flip_channel( exp_pa.getChannel() ) )
            delay = received_data[i].getTime() - pulse_events[i].getTime()
            print rpa, ", delay:", delay
            self.assertEqual( rpa, exp_pa, "Expected {exp}, actual {act}".format(exp=exp_pa, act=rpa))
            delays.append(delay)

        min_delay = np.min(delays)
        max_delay = np.max(delays)
        self.assertTrue( 80 < min_delay and max_delay < 120, "Loopback delays between pulses should in range (80, 120) clock cycles")


if __name__ == '__main__':
    #PlaybackTraceHWTest().runTest()
    unittest.main()
