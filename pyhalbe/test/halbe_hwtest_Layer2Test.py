#!/usr/bin/env python
from HWTest import HWTest
from pyhalbe import *
from pyhalco_common import Enum
import pyhalco_hicann_v2 as Coordinate
import unittest
import time

def flip_GbitLink(channel):
    """
    flips GbitLinkOnHICANN as it is expectd after loopback
    """
    assert isinstance(channel, Coordinate.GbitLinkOnHICANN)
    nn = int(channel)
    return Coordinate.GbitLinkOnHICANN( int(nn/2)*2 + (not(nn%2)))

class Layer2HWTests(HWTest):
    """
    Layer 2 Pulse tests.
    Including Tests of playback fifo and external DDR2 playback and trace memory.
    Pulses are looped back in either the DNC or the HICANN.
    Currently only a single HICANN is supported!
    """

    def comparePulseLists(self, expected, actual):
        """
	    compare two pulse lists for being exactly equal.
        """
        assert isinstance(expected, FPGA.PulseEventContainer)
        assert isinstance(actual, FPGA.PulseEventContainer)
        self.assertEqual(expected.size(),actual.size())
        for n in range(min(expected.size(), actual.size())):
            self.assertEqual(expected[n], actual[n])

    def comparePulseListsAddress(self, expected, actual, flip_channel=False):
        """
	    compare two pulse lists for having the same addresses
        params:
            flip_channel - boolean, if True the dnc interface channel of the expected address is flipped.
                           i.e. from 0->1, 1->0, 2->3, 3->2, ...
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

    def dnc_loopback(self):
        gbit = DNC.GbitReticle()
        # no timestamp handling
        # all directions set TO_HICANN, to make sure there are no spikes interfering from the HICANNs
        for i in range(8):
            hc = Coordinate.HICANNOnDNC(Enum(i))
            gbit[hc].timestamp_enable = False
            for j in range(8):
			    gbit[hc].dirs[j] = HICANN.GbitLink.Direction.TO_HICANN

        DNC.set_hicann_directions(self.fpga, self.dnc, gbit)

        loopback = DNC.Loopback()
        loopback[self.h.to_HICANNOnDNC()] = True

        DNC.set_loopback(self.fpga, self.dnc, loopback)

    def hicann_loopback(self):
        """
        sets loopback on HICANN 0, such that channel 0->1, 2->3, etc.
        """

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
        HICANN.set_dnc_merger(self.h, mergers)

        gbit = DNC.GbitReticle()

        link = HICANN.GbitLink()
        for i in range(8):
            if (i%2):
                link.dirs[i] = HICANN.GbitLink.Direction.TO_DNC
            else:
                link.dirs[i] = HICANN.GbitLink.Direction.TO_HICANN

	    gbit[self.h.to_HICANNOnDNC()] = link # only HICANN 0

	    HICANN.set_gbit_link(self.h, link)
        DNC.set_hicann_directions(self.fpga, self.dnc, gbit)

        # make sure to switch off dnc loopback
        DNC.set_loopback(self.fpga, self.dnc, DNC.Loopback() )

    def run_playback_and_trace_experiment(self, input_pulses, run_time_in_us):
        """
        runs an experiment with playback and trace memory
        and returns the traced pulses
        params:
            input_pulses  - FPGA.PulseEventContainer with input pulses
            run_time_in_us - experiment duration in micro seconds
        returns
            FPGA.PulseEventContainer with traced pulses
        """

        ##########################
        # typical calling sequence for playback experiment!
        #
        runtime_in_dnc_cycles = run_time_in_us / FPGA.DNC_frequency_in_MHz
        FPGA.write_playback_pulses(
                self.fpga,
                input_pulses,
                runtime_in_dnc_cycles,
                63 # fpga_hicann_delay, 63 fpga_clk cycles are 504 nano seconds
                )
        FPGA.prime_experiment(self.fpga);
        FPGA.start_experiment(self.fpga);
        received_data = FPGA.read_trace_pulses(self.fpga, runtime_in_dnc_cycles).events
        #
        ##########################
        return received_data

    def test_PulseFifoHICANNLoopbackTest(self):
        self.hicann_loopback()
        hicann_on_dnc = self.h.to_HICANNOnDNC()

        pulse_events = []
        pulse_address = FPGA.PulseAddress(self.dnc, hicann_on_dnc, Coordinate.GbitLinkOnHICANN(2),HICANN.Neuron.address_t(63))
        #  5000 Pulses with ISI = 500 clks (2000 ns)
        isi = 50 # every 500 cycles
        start_offset = 500
        num_pulses = 500
        for np in range(num_pulses):
            pulse_events.append(FPGA.PulseEvent(pulse_address, isi*(np)+start_offset))

        run_time_in_us = pulse_events[len(pulse_events)-1].getTime() / FPGA.DNC_frequency_in_MHz
        received_data = FPGA.send_and_receive(self.fpga, self.dnc, FPGA.PulseEventContainer(pulse_events), False, int(run_time_in_us))

        # sent and received pulses should be exactly equal - including timestamps!
        self.comparePulseListsAddress(FPGA.PulseEventContainer(pulse_events),received_data, flip_channel=True)

    def test_PlaybackTraceHICANNLoopbackTest(self):
        self.hicann_loopback()
        hicann_on_dnc = self.h.to_HICANNOnDNC()

        pulse_events = []
        pulse_address = FPGA.PulseAddress(self.dnc,hicann_on_dnc,Coordinate.GbitLinkOnHICANN(0),HICANN.Neuron.address_t(63))
        #  5000 Pulses with ISI = 500 clks (2000 ns)
        isi = 50 # every 500 cycles
        start_offset = 500
        num_pulses = 5000
        for np in range(num_pulses):
            pulse_events.append(FPGA.PulseEvent(pulse_address, isi*(np)+start_offset))

        run_time_in_us = pulse_events[len(pulse_events)-1].getTime() / FPGA.DNC_frequency_in_MHz + 1.e4
        received_data = self.run_playback_and_trace_experiment(FPGA.PulseEventContainer(pulse_events),run_time_in_us)

        # addresses sent and received pulses should be identical, only the channel flips
        self.comparePulseListsAddress(FPGA.PulseEventContainer(pulse_events),received_data, flip_channel=True)

    def test_MultiplePlaybackTraceHICANNLoopbackTest(self):
        """
        tests whether playback can run multiple times.
        using hicann loopback.
        """
        self.hicann_loopback()
        hicann_on_dnc = self.h.to_HICANNOnDNC()

        for i in range(3):
            pulse_events = []
            pulse_address = FPGA.PulseAddress(self.dnc,hicann_on_dnc,Coordinate.GbitLinkOnHICANN(0),HICANN.Neuron.address_t(i))
            #  5000 Pulses with ISI = 500 clks (2000 ns)
            isi = 50 # every 500 cycles
            start_offset = 500
            num_pulses = 5000
            for np in range(num_pulses):
                pulse_events.append(FPGA.PulseEvent(pulse_address, isi*(np)+start_offset))

            run_time_in_us = pulse_events[len(pulse_events)-1].getTime() / FPGA.DNC_frequency_in_MHz + 1.e4
            received_data = self.run_playback_and_trace_experiment(FPGA.PulseEventContainer(pulse_events),run_time_in_us)
            self.assertEqual(len(pulse_events), received_data.size(), "run %s fails"%i)

            # addresses sent and received pulses should be identical, only the channel flips
            self.comparePulseListsAddress(FPGA.PulseEventContainer(pulse_events),received_data, flip_channel=True)

    #@unittest.skip("")
    def test_SimpleSpinnakerIFHWTest(self):
        """
        Test description:
            Layer 2 is in HICANN loopback mode
            We then configure the Spinnaker IF,
            send pulses via the spinnaker pulse IF
            and finally try to receive spikes on the other side.
        """
        self.hicann_loopback()
        hicann_on_dnc = self.h.to_HICANNOnDNC()

        # we need to prepend one empty experiment
        self.run_playback_and_trace_experiment(FPGA.PulseEventContainer(), run_time_in_us=100)

        pulse_address = FPGA.PulseAddress(self.dnc,hicann_on_dnc,Coordinate.GbitLinkOnHICANN(0),HICANN.Neuron.address_t(25))

        up_sample_count = 100
        down_sample_count = 10

        # set up spinnaker stuff:
        FPGA.set_spinnaker_pulse_upsampler(self.fpga, up_sample_count)
        FPGA.set_spinnaker_pulse_downsampler(self.fpga, down_sample_count)

        # routing table
        spinn_pa = FPGA.SpinnInputAddress_t(0)
        routing_table = FPGA.SpinnRoutingTable()
        routing_table.set(spinn_pa, pulse_address)
        FPGA.set_spinnaker_routing_table(self.fpga, routing_table)

        # sender foncif
        """
        ssc = FPGA.SpinnSenderConfig()
        ssc.setActive(True)
        ssc.setTargetIP(Coordinate.IPv4.from_string("192.168.1.127"))
        ssc.setTargetPort(1851)
        FPGA.set_spinnaker_sender_config(self.fpga, ssc)
        """
        # WE SEND ONLY 1 PULSE
        FPGA.add_spinnaker_pulse(self.fpga, spinn_pa)
        FPGA.send_spinnaker_pulses(self.fpga)
        time.sleep(0.0002)

        # expecte pulse address is configured pulse address with dnc if channel flipped
        exp_pa = FPGA.PulseAddress(pulse_address)
        exp_pa.setChannel( flip_GbitLink(exp_pa.getChannel()) )
        # There should be exactly up_sample_count/down_sample_count pulses
        num_expected_rec_pulses = int(up_sample_count/down_sample_count)
        for i in range(num_expected_rec_pulses):
            received_address = FPGA.get_received_spinnaker_pulse(self.fpga)
            rpa = FPGA.PulseAddress(int(received_address))
            self.assertEqual( rpa, exp_pa, "Expected {exp}, actual {act}".format(exp=exp_pa, act=rpa))
        self.assertRaises( RuntimeError, FPGA.get_received_spinnaker_pulse, self.fpga )

    @unittest.skip("needs root permissions...")
    def test_SimpleRealtimeSpinnakerIFHWTest(self):
        """
        Test description:
            Layer 2 is in HICANN loopback mode
            We then configure the Spinnaker IF,
            send pulses via the realtime spinnaker pulse IF
            and finally try to receive spikes on the other side.
        """
        self.hicann_loopback()
        hicann_on_dnc = self.h.to_HICANNOnDNC()

        # we need to prepend one empty experiment
        self.run_playback_and_trace_experiment(FPGA.PulseEventContainer(), run_time_in_us=100)

        pulse_address = FPGA.PulseAddress(self.dnc,hicann_on_dnc,Coordinate.GbitLinkOnHICANN(0),HICANN.Neuron.address_t(25))

        up_sample_count = 100
        down_sample_count = 10

        # set up spinnaker stuff:
        FPGA.set_spinnaker_pulse_upsampler(self.fpga, up_sample_count)
        FPGA.set_spinnaker_pulse_downsampler(self.fpga, down_sample_count)

        # routing table
        spinn_pa = FPGA.SpinnInputAddress_t(0)
        routing_table = FPGA.SpinnRoutingTable()
        routing_table.set(spinn_pa, pulse_address)
        FPGA.set_spinnaker_routing_table(self.fpga, routing_table)

        # WE SEND ONLY 1 PULSE
        spinn_pa = Realtime.spike_h()
        spinn_pa.set_label(spinn_pa, 0) # uh, interface FIXME
        FPGA.send_spinnaker_realtime_pulse(self.fpga, spinn_pa)
        time.sleep(0.0002)

        # expecte pulse address is configured pulse address with dnc if channel flipped
        exp_pa = FPGA.PulseAddress(pulse_address)
        exp_pa.setChannel( flip_GbitLink(exp_pa.getChannel()) )
        # There should be exactly up_sample_count/down_sample_count pulses
        num_expected_rec_pulses = int(up_sample_count/down_sample_count)

        sl = FPGA.get_received_realtime_pulses(self.fpga)
        self.assertEqual(len(sl), num_expected_rec_pulses)

        for sp in sl:
            rpa = FPGA.PulseAddress(int(sp))
            self.assertEqual( rpa, exp_pa, "Expected {exp}, actual {act}".format(exp=exp_pa, act=rpa))

    @unittest.skip("")
    def test_IssueSpinnIFSendsOldPulsesHWTest(self):
        """
        Test description:
            Minimally adopted test from SimpleSpinnakerIFTest
            Fails for the Issue, that if there are pulses available
            in the FPGA upstream side, before the first UDP-Pulse arrived
            in the FPGA: The prior pulses than fill the SpinnIF out fifo and
            are then sent as soon as the SpinnIF gets activated.
            we reproduce this by running a simple loopback experiment with 10
            pulses, and then change the pulse address for teh normal spinn If
            test.
        """
        self.hicann_loopback()
        hicann_on_dnc = self.h.to_HICANNOnDNC()

        ################################################
        # we first test, whether the L2 connection works
        ################################################
        pulse_events = []
        pulse_address = FPGA.PulseAddress(self.dnc,hicann_on_dnc,Coordinate.GbitLinkOnHICANN(0),HICANN.Neuron.address_t(0))
        #  5000 Pulses with ISI = 500 clks (2000 ns)
        isi = 50 # every 500 cycles
        start_offset = 500
        num_pulses = 10
        for np in range(num_pulses):
            pulse_events.append(FPGA.PulseEvent(pulse_address, isi*(np)+start_offset))

        run_time_in_us = pulse_events[len(pulse_events)-1].getTime() / FPGA.DNC_frequency_in_MHz
        received_data = self.run_playback_and_trace_experiment(FPGA.PulseEventContainer(pulse_events),run_time_in_us)

        # addresses sent and received pulses should be identical, only the channel flips
        self.comparePulseListsAddress(FPGA.PulseEventContainer(pulse_events),received_data, flip_channel=True)
        #  L2 test done
        ################################################

        # we now change the address
        pulse_address = FPGA.PulseAddress(self.dnc,hicann_on_dnc,Coordinate.GbitLinkOnHICANN(0),HICANN.Neuron.address_t(1))
        up_sample_count = 100
        down_sample_count = 10

        # set up spinnaker stuff:
        FPGA.set_spinnaker_pulse_upsampler(self.fpga, up_sample_count)
        FPGA.set_spinnaker_pulse_downsampler(self.fpga, down_sample_count)

        # routing table
        spinn_pa = FPGA.SpinnInputAddress_t(0)
        routing_table = FPGA.SpinnRoutingTable()
        routing_table.set(spinn_pa, pulse_address)
        FPGA.set_spinnaker_routing_table(self.fpga, routing_table)

        FPGA.add_spinnaker_pulse(self.fpga, spinn_pa)
        FPGA.send_spinnaker_pulses(self.fpga)
        time.sleep(0.0002)

        # expecte pulse address is configured pulse address with dnc if channel flipped
        exp_pa = FPGA.PulseAddress(pulse_address)
        exp_pa.setChannel( flip_GbitLink( exp_pa.getChannel()) )
        num_expected_rec_pulses = int(up_sample_count/down_sample_count)
        for i in range(num_expected_rec_pulses):
            received_address = FPGA.get_received_spinnaker_pulse(self.fpga)
            rpa = FPGA.PulseAddress(int(received_address))
            self.assertEqual( rpa, exp_pa, "Expected {exp}, actual {act}".format(exp=exp_pa, act=rpa))
        self.assertRaises( RuntimeError, FPGA.get_received_spinnaker_pulse, self.fpga )

    #@unittest.skip("")
    def test_SpinnakerIF2TraceHWTest(self):
        """
        Test description:
            Layer 2 is in HICANN loopback mode
            We then configure the Spinnaker IF,
            send pulses via the spinnaker pulse IF
            and finally try to receive spikes on the playback
        """
        self.hicann_loopback()
        hicann_on_dnc = self.h.to_HICANNOnDNC()

        ## we need to prepend one empty experiment
        #self.run_playback_and_trace_experiment(FPGA.PulseEventContainer(), run_time_in_us=100)


        pulse_address = FPGA.PulseAddress(self.dnc, hicann_on_dnc, Coordinate.GbitLinkOnHICANN(0),HICANN.Neuron.address_t(25))

        up_sample_count = 100
        down_sample_count = 10

        # set up spinnaker stuff:
        FPGA.set_spinnaker_pulse_upsampler(self.fpga, up_sample_count)
        FPGA.set_spinnaker_pulse_downsampler(self.fpga, down_sample_count)

        # routing table
        spinn_pa = FPGA.SpinnInputAddress_t(42)
        routing_table = FPGA.SpinnRoutingTable()
        routing_table.set(spinn_pa, pulse_address)
        FPGA.set_spinnaker_routing_table(self.fpga, routing_table)

        runtime_in_us = 400
        runtime_in_dnc_cycles = runtime_in_us * FPGA.DNC_frequency_in_MHz
        FPGA.write_playback_pulses(self.fpga, FPGA.PulseEventContainer(), runtime_in_dnc_cycles, 63)
        FPGA.prime_experiment(self.fpga)
        FPGA.start_experiment(self.fpga)
        time.sleep(runtime_in_us / 2)

        # WE SEND ONLY 1 PULSE
        FPGA.add_spinnaker_pulse(self.fpga, spinn_pa)
        FPGA.send_spinnaker_pulses(self.fpga)
        time.sleep(runtime_in_us / 2)

        received_data = FPGA.read_trace_pulses(self.fpga, runtime_in_dnc_cycles).events
        self.assertTrue(received_data.size()>0)

if __name__ == '__main__':
    Layer2HWTests.main()
