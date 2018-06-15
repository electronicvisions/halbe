#!/usr/bin/env python
from HWTest import HWTest
from pyhalbe import *
import unittest
import time
import numpy as np

import pylogging
pylogging.reset()
pylogging.default_config(level=pylogging.LogLevel.INFO)

DNCOnFPGA = Coordinate.DNCOnFPGA
HICANNOnDNC = Coordinate.HICANNOnDNC
GbitLinkOnHICANN = Coordinate.GbitLinkOnHICANN
Enum = Coordinate.Enum


def filter_events_from_hicann(pc,hicann):
    """returns pulse list with only spikes from one hicann"""
    assert isinstance(pc, FPGA.PulseEventContainer)
    assert isinstance(hicann, HICANNOnDNC)

    rv = []
    for i in range(pc.size()):
        e = pc[i]
        if e.getChipAddress() == hicann:
            rv.append(e)
    return FPGA.PulseEventContainer(rv)

def flip_GbitLink(channel):
    """
    flips GbitLinkOnHICANN as it is expectd after loopback
    """
    assert isinstance(channel, Coordinate.GbitLinkOnHICANN)
    nn = int(channel)
    return Coordinate.GbitLinkOnHICANN( int(nn/2)*2 + (not(nn%2)))


def hicann_loopback(fpga, h, dnc):
    """
    sets loopback on HICANN, such that channel 0->1, 2->3, etc.
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

    gbit = DNC.GbitReticle()

    link = HICANN.GbitLink()
    for i in range(8):
        if (i%2):
            link.dirs[i] = HICANN.GbitLink.Direction.TO_DNC
        else:
            link.dirs[i] = HICANN.GbitLink.Direction.TO_HICANN

    gbit[h.to_HICANNOnDNC()] = link

    HICANN.set_gbit_link(h, link)
    DNC.set_hicann_directions(fpga, dnc, gbit)

    # make sure to switch off dnc loopback
    DNC.set_loopback(fpga, dnc, DNC.Loopback() )

#FIXME synchronization between sending of spike train an experiment start needs
#      to be implemented as well as a proper marking of experiment end
#      see issue 2174

class PlaybackTraceHWTest(HWTest):
    """
    collection of tests for the playback and trace modules.

    Up to now, all tests are HICANN loopback tests.
    Multiple runs without reset are implicitly tested in many tests.
    """

    # expected loop back delays in DNC clokcs for PLL = 100 MHz, cf. #1310 and #1820
    exp_delay_kintex = 71
    exp_delay_virtex = 102

    # FPGA HICANN delay in FPGA clocks (8ns)
    fpga_hicann_delay_kintex = 25
    fpga_hicann_delay_virtex = 40

    extra_time_in_us = 1.e4 # 10 ms

    def getFpgaHicannDelay(self):
        if self.fpga.isKintex():
            return self.fpga_hicann_delay_kintex
        else:
            return self.fpga_hicann_delay_virtex


    def comparePulseListsAddress(self, expected, actual, flip_channel=False):
        """
	    compare two pulse lists for having the same addresses
        params:
            flip_channel - boolean, if True the dnc interface channel of the
                           expected address is flipped.  i.e. from 0->1, 1->0,
                           2->3, 3->2, ...
        """
        assert isinstance(expected, FPGA.PulseEventContainer)
        assert isinstance(actual, FPGA.PulseEventContainer)
        assert isinstance(flip_channel, bool)
        self.assertEqual(expected.size(),actual.size())
        for n in range(min(expected.size(), actual.size())):
            exp_addr = FPGA.PulseAddress(expected[n])
            if flip_channel:
                exp_addr.setChannel(flip_GbitLink(exp_addr.getChannel()))
            act_addr = FPGA.PulseAddress(actual[n])
            self.assertEqual(exp_addr, act_addr, "expected: %s, actual: %s" % (exp_addr, act_addr))

    def computeDelays(self, expected, actual):
        """
	    computes the delays between two pulse lists, which must be of same size.
        """
        assert isinstance(expected, FPGA.PulseEventContainer)
        assert isinstance(actual, FPGA.PulseEventContainer)
        self.assertEqual(expected.size(),actual.size())

        delays = np.zeros(actual.size(), dtype=int)
        for i in range(actual.size()):
            delay = actual[i].getTime() - expected[i].getTime()
            delays[i] = delay
        return delays

    def checkMeanDelay(self, expected, actual, fpga_hicann_delay=None, delta=10):
        delays = self.computeDelays(expected, actual)
        mean_delay = delays.mean()

        if fpga_hicann_delay is not None:
            f_h_delay = fpga_hicann_delay
        else:
            f_h_delay = self.getFpgaHicannDelay()

        if self.fpga.isKintex():
            loopback_delay = self.exp_delay_kintex
        else:
            loopback_delay = self.exp_delay_virtex

        exp_delay = loopback_delay - 2*f_h_delay # factor 2: convert fpga to dnc clocks

        self.assertGreater(mean_delay, exp_delay-delta)
        self.assertLess(mean_delay,exp_delay+delta)


    def genRegularSpiketrain(self, num_pulses, isi, start_offset, glink, neuron):
        pulse_address = FPGA.PulseAddress(
                self.dnc,
                self.h.to_HICANNOnDNC(),
                GbitLinkOnHICANN(glink),
                HICANN.Neuron.address_t(neuron))
        pulse_events = []
        for i in range(num_pulses):
            pulse_events.append(FPGA.PulseEvent(pulse_address, isi*(i)+start_offset))
        return FPGA.PulseEventContainer(pulse_events)

    def genRegularSpiketrainRandAddress(self, num_pulses, isi, start_offset, link_parity=0):
        assert link_parity in [0,1]

        pulse_events = []
        for i in range(num_pulses):
            pulse_events.append(
                    FPGA.PulseEvent(
                    self.dnc,
                    self.h.to_HICANNOnDNC(),
                    GbitLinkOnHICANN(np.random.randint(4)*2 + link_parity),
                    HICANN.Neuron.address_t(np.random.randint(64)),
                    isi*(i)+start_offset))
        return FPGA.PulseEventContainer(pulse_events)

    def genPoissonSpiketrainRandAddress(self, num_pulses, isi, start_offset, link_parity=0):
        assert link_parity in [0,1]

        times = np.cumsum(np.random.exponential(isi, num_pulses))
        times = times.astype(int)

        pulse_events = []
        for t in times:
            pulse_events.append(
                    FPGA.PulseEvent(
                    self.dnc,
                    self.h.to_HICANNOnDNC(),
                    GbitLinkOnHICANN(np.random.randint(4)*2 + link_parity),
                    HICANN.Neuron.address_t(np.random.randint(64)),
                    int(t+start_offset) ))
        return FPGA.PulseEventContainer(pulse_events)

    def genTruncatedPoissonSpiketrainRandAddress(self, num_pulses, isi, start_offset, link_parity=0):
        """
        generate a truncated Poisson spike train, as it is done for the old
        virtex bitfile, where pulses are shifted in hicann-system to have a
        minimum ISI of 6 fpga clocks.
        """
        assert link_parity in [0,1]

        times = np.cumsum(np.random.exponential(isi, num_pulses))
        times = times.astype(int)
        min_isi = 12 # time difference between two pulse groups in FPGA
        times_shifted = np.zeros(times.shape, dtype=int)
        last_t = -min_isi
        for n,t in enumerate(times):
            if (t - last_t) < min_isi:
                times_shifted[n] = last_t + min_isi
            else:
                times_shifted[n] = t
            last_t = times_shifted[n]

        pulse_events = []
        for t in times_shifted:
            pulse_events.append(
                    FPGA.PulseEvent(
                    self.dnc,
                    self.h.to_HICANNOnDNC(),
                    GbitLinkOnHICANN(np.random.randint(4)*2 + link_parity),
                    HICANN.Neuron.address_t(np.random.randint(64)),
                    t+start_offset ))
        return FPGA.PulseEventContainer(pulse_events)

    def runExperiment(self, input_pulses, run_time_in_us=None,
            fpga_hicann_delay=None):
        """
        runs an experiment with playback and trace memory
        and returns the traced pulses
        params:
            input_pulses  - FPGA.PulseEventContainer with input pulses
            run_time_in_us - experiment duration in micro seconds. If not
                             passed, the duration is taken from the last spike
                             time plus some extra time.
            fpga_hicann_delay - FPGA HICANN latency in FPGA clock cycles (8ns)
                                If not passed, a default value is used
                                depending on the system.
        returns
            FPGA.PulseEventContainer with traced pulses
        """

        if run_time_in_us is not None:
            duration = run_time_in_us
        else:
            duration = input_pulses[input_pulses.size()-1].getTime() / FPGA.DNC_frequency_in_MHz \
                        + self.extra_time_in_us

        if fpga_hicann_delay is not None:
            f_h_delay = fpga_hicann_delay
        else:
            f_h_delay = self.getFpgaHicannDelay()

        runtime_in_dnc_cycles = duration * FPGA.DNC_frequency_in_MHz
        FPGA.write_playback_pulses(self.fpga, input_pulses, runtime_in_dnc_cycles, f_h_delay)
        FPGA.prime_experiment(self.fpga)
        FPGA.start_experiment(self.fpga)

        return FPGA.read_trace_pulses(self.fpga, runtime_in_dnc_cycles).events

    @unittest.expectedFailure
    def test_basic(self):
        """test basic behavior of playback and trace"""
        hicann_loopback(self.fpga, self.h, self.dnc)

        pc =  self.genRegularSpiketrainRandAddress(
                num_pulses=200, isi=50, start_offset=500)

        received_data = self.runExperiment(pc,fpga_hicann_delay=0)

        # expected pulse address is the configured pulse address with dnc if channel flipped
        self.comparePulseListsAddress(pc,received_data, flip_channel=True)

        delays = self.computeDelays(pc,received_data)

        if self.fpga.isKintex():
            exp_delay = self.exp_delay_kintex
        else:
            exp_delay = self.exp_delay_virtex
        acc_dev = 20 # acceptable deviation of 20 clock cycles.
        self.assertTrue( exp_delay - acc_dev  < delays.min() and delays.max() < exp_delay + acc_dev, "Loopback delays are out of range")

    @unittest.expectedFailure
    def test_rate_sweep_regular(self):
        """test a huge range of different regular spike trains"""
        hicann_loopback(self.fpga, self.h, self.dnc)

        # Tests rates between 5k and 17.8M event/s
        isis = [14, 16, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000]
        for isi in isis:
            pc =  self.genRegularSpiketrainRandAddress(
                    num_pulses=5000, isi=isi, start_offset=500)

            received_data = self.runExperiment(pc)

            self.comparePulseListsAddress(pc,received_data, flip_channel=True)
            self.checkMeanDelay(pc,received_data)

    @unittest.expectedFailure
    def test_rate_sweep_poisson(self):
        """
        test a huge range of different poisson spike trains
        
        For Poisson spike trains there can be spike loss also for rates lower
        than the maximum rate. Therefore, we check whether the loss is below some
        self-defined limit depending on the sent ISI (see below).
        """
        hicann_loopback(self.fpga, self.h, self.dnc)

        # Tests rates between 5k and 17.8M event/s
        isis = [14, 16, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000]
        for isi in isis:
            pc =  self.genPoissonSpiketrainRandAddress(
                    num_pulses=50000, isi=isi, start_offset=500)

            received_data = self.runExperiment(pc)

            min_isi = 14 # min ISI in regular mode
            # allowed_loss = probability that two subsequent ISIs are lower
            # than minimum supplied ISI in regular mode
            # -> allowed_loss = CDF(x=min_isi, scale=isi)**2
            # CDF: cumulative distribution function of negative exponential distr.
            allowed_loss = (1 - np.exp(-min_isi*1./isi))**2
            rel_loss = (pc.size() - received_data.size())*1./pc.size()
            print allowed_loss, rel_loss
            self.assertLess(rel_loss,allowed_loss)

    @unittest.expectedFailure
    def test_late_experiment_start(self):
        """test late experiment start. after 2 loops of counter"""
        hicann_loopback(self.fpga, self.h, self.dnc)

        overflow = 2**15
        start = 2*overflow + 500
        pc =  self.genRegularSpiketrainRandAddress(
                num_pulses=200, isi=50, start_offset=start)

        received_data = self.runExperiment(pc)

        self.comparePulseListsAddress(pc,received_data, flip_channel=True)
        self.checkMeanDelay(pc,received_data)

    @unittest.expectedFailure
    def test_large_isi(self):
        """test isi with up to 3 overflows. also tests multiple runs. """
        hicann_loopback(self.fpga, self.h, self.dnc)

        overflow = 2**15
        for i in range(4):
            isi = i*overflow + 2**12
            pc =  self.genRegularSpiketrainRandAddress(
                    num_pulses=200, isi=isi, start_offset=500)

            received_data = self.runExperiment(pc)

            self.comparePulseListsAddress(pc,received_data, flip_channel=True)
            self.checkMeanDelay(pc,received_data)

    @unittest.expectedFailure
    def test_several_pulses_per_frame(self):
        """test several pulses per frame """
        hicann_loopback(self.fpga, self.h, self.dnc)

        # we use a mean isi of 20
        mean_isi = 20

        for isi1 in range(1,mean_isi+1):
            isi2 = 2*mean_isi - isi1
            pulse_events = []
            num_pulses = 100
            t = 500 # last spike time, initialized with some start offset
            for i in range(num_pulses):
                addr = FPGA.PulseAddress(
                        self.dnc,
                        self.h.to_HICANNOnDNC(),
                        GbitLinkOnHICANN(np.random.randint(4)*2),
                        HICANN.Neuron.address_t(np.random.randint(64)))
                if i%2:
                    t =  t + isi1
                else:
                    t =  t + isi2
                pulse_events.append(FPGA.PulseEvent(addr,t))

            pc = FPGA.PulseEventContainer(pulse_events)
            received_data = self.runExperiment(pc)

            self.comparePulseListsAddress(pc,received_data, flip_channel=True)
            self.checkMeanDelay(pc,received_data)

    @unittest.expectedFailure
    def test_issue1870(self):
        """
        reproduces issue 1870:
        2nd pulse of two subsequent pulses to same HICANN in same pulse group
        is lost.
        """
        hicann_loopback(self.fpga, self.h, self.dnc)

        pulse_events = []

        # tested hicann
        a1 = FPGA.PulseAddress(
                    self.dnc,
                    self.h.to_HICANNOnDNC(),
                    GbitLinkOnHICANN(0),
                    HICANN.Neuron.address_t(0))
        # different HICANN
        a2 = FPGA.PulseAddress(
                    self.dnc,
                    HICANNOnDNC(Enum((int(self.h.to_HICANNOnDNC().id()) + 1 )%8)),
                    GbitLinkOnHICANN(0),
                    HICANN.Neuron.address_t(0))
        # tested hicann, different glink
        a3 = FPGA.PulseAddress(
                    self.dnc,
                    self.h.to_HICANNOnDNC(),
                    GbitLinkOnHICANN(2),
                    HICANN.Neuron.address_t(0))

        PE = FPGA.PulseEvent
        pulses = [PE(a1,500), PE(a3,502)]

        # If a pulse to another hicann is added in between, no pulses are lost.
        #pulses = [PE(a1,500), PE(a2, 501), PE(a3,502)]

        for pulse in pulses:
            pulse_events.append(pulse)

        pc = FPGA.PulseEventContainer(pulse_events)
        received_data = self.runExperiment(pc)

        print "Sent pulses"
        for i in range(pc.size()):
            print pc[i]

        print "Received pulses"
        for i in range(received_data.size()):
            print received_data[i]

        pulses_this_hicann =  filter_events_from_hicann(pc,self.h.to_HICANNOnDNC())

        self.comparePulseListsAddress(pulses_this_hicann,received_data, flip_channel=True)
        self.checkMeanDelay(pulses_this_hicann,received_data)

    @unittest.expectedFailure
    def test_playback_supplies_max_rate(self):
        """
        tests whether the playback can supply the maximum available rate.

        The playback can send one pulse per FPGA clock cycle.
        Whenever a new pulse group starts, it takes 6 cycles.
        A maximum of 184 pulses can be part of one pulse group.
        This yields the maximum rate: 184 spikes per 184+6 fpga clock cycles.

        We send pulses with the maximum theoretical rate in a round-robin
        manner to all 8 HICANNs, but only loopback and trace the pulses from
        one hicann. Theses pulses are than compared to the pulses sent to this
        HICANN. We also check whether the mean loopback delay stays constant.
        A total of 400.000 pulses are sent.
        """
        hicann_loopback(self.fpga, self.h, self.dnc)
        # numbers from hicann-system
        max_pulses_per_group = 184
        min_reltime_distance_clks = 6 #  fpga clocks

        min_sustained_isi_fpga = (max_pulses_per_group + min_reltime_distance_clks)*1./max_pulses_per_group
        min_sustained_isi_dnc = 2*min_sustained_isi_fpga

        pulse_events = []

        isi=min_sustained_isi_dnc
        hicanns = 8
        PE = FPGA.PulseEvent
        num_pulses = 50000*hicanns
        start_offset = 500

        for i in range(num_pulses):
            pulse_events.append(
                    FPGA.PulseEvent(
                    self.dnc,
                    HICANNOnDNC(Enum(i%hicanns)),
                    GbitLinkOnHICANN(np.random.randint(4)*2),
                    HICANN.Neuron.address_t(np.random.randint(64)),
                    int(np.ceil(isi*(i)+start_offset))))

        pc = FPGA.PulseEventContainer(pulse_events)
        received_data = self.runExperiment(pc)

        pulses_this_hicann =  filter_events_from_hicann(pc,self.h.to_HICANNOnDNC())

        self.comparePulseListsAddress(pulses_this_hicann,received_data, flip_channel=True)
        self.checkMeanDelay(pulses_this_hicann,received_data)

if __name__ == '__main__':
    PlaybackTraceHWTest.main()
