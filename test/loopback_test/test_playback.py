#!/usr/bin/env python
import pyoneer
from pyhalbe import *
from HWTest import HWTest
import unittest
import time
import copy
import numpy.random as npr
import cPickle


def hicann_loopback(fpga, h, dnc, hicann_no=0):
    """
    sets loopback on HICANN 0, such that channel 0->1, 2->3, etc.
    """
    assert isinstance(h, Handle.HICANN)
    dnc = fpga.dnc(dnc)
    assert isinstance(dnc, Coordinate.DNCGlobal)

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

    gbit[hicann_no] = link

    HICANN.set_gbit_link(h, link)
    DNC.set_hicann_directions(dnc, gbit)

    # make sure to switch off dnc loopback
    DNC.set_loopback(dnc, DNC.loopback_t() )

class PlaybackTraceHWTest(HWTest):
    def runTest(self):
        results = []
        experiment_no = 0
        #for random_interval in [ False, True ]:
        for random_interval in [ True ]:
            #for isi in [ 500, 250, 125, 62, 31 ]:
            for isi in [ 62 ]:
                #for num_pulses in [ 1, 5000, 500000, 1000000 ]:
                for num_pulses in [ 1000000 ]:
                    for repetition in range(4):
                        print
                        print "random_interval = %s" % random_interval
                        print "isi = %d" % isi
                        print "num_pulses = %d" % num_pulses
                        print

                        ########################
                        #  configure hardware for hicann loopback
                        ########################
                        FPGA.reset(self.fpga)
                        HICANN.reset(self.h) # resets FPGA
                        HICANN.init(self.h)

                        FPGA.sync_systime_counters(self.h)

                        hicann_loopback(self.fpga, self.h, self.dnc, hicann_no=self.HICANN)

                        ########################
                        #  generate input data
                        ########################

                        #  5000 Pulses with ISI = 500 clks (2000 ns)
                        pulse_address = FPGA.PulseAddress(self.DNC, self.HICANN, 0,HICANN.Neuron.address_t(25))
                        pulse_events = []
                        #isi = 50 # every 500 cycles
                        start_offset = 500
                        cur_t = copy.copy(start_offset)
                        #num_pulses = 5000
                        for np in range(num_pulses):
                            #pulse_events.append(FPGA.PulseEvent(pulse_address, isi*(np)+start_offset))
                            n = npr.randint(64)
                            nrn_address = HICANN.Neuron.address_t(n)
                            pulse_address = FPGA.PulseAddress(self.DNC,
                                                              self.HICANN,
                                                              0,
                                                              nrn_address)
                            pulse_events.append(FPGA.PulseEvent(pulse_address, cur_t))

                            if random_interval:
                                cur_t += npr.poisson(isi)
                            else:
                                cur_t += isi

                        pc = FPGA.PulseEventContainer(pulse_events)


                        ########################
                        # run experiment
                        ########################

                        run_time_dnc_cycles = pc[pc.size()-1].getTime() + 100
                        run_time_in_us = run_time_dnc_cycles / FPGA.DNC_frequency_in_MHz
                        FPGA.write_playback_pulses(
                                self.fpga,
                                pc,
                                run_time_dnc_cycles
                                63 # fpga_hicann_delay, 63 fpga_clk cycles are 504 nano seconds
                                )
                        FPGA.prime_experiment(self.fpga)
                        FPGA.start_experiment(self.fpga)
                        received_data = FPGA.read_trace_pulses(self.fpga, run_time_dnc_cycles).events

                        ########################
                        #  compare data
                        ########################

                        # There should be as many 
                        self.assertEqual(pc.size(), received_data.size())
                        if not pc.size() == received_data.size():
                            print "*** got: %d, expected: %d ***" % (received_data.size(),
                                                                     pc.size())

                        train_mismatch = 0 
                        mismatch_at = -1
                        for i in range(min(pc.size(), received_data.size())):
                            a = received_data[i].getNeuronAddress()
                            b = pc[i].getNeuronAddress()
                            if a != b:
                                #print "*** mismatch in spiketrains at %d ***" % (i)
                                #print "*** got: ", FPGA.PulseAddress(received_data[i])
                                #print "*** expected: ", FPGA.PulseAddress(pc[i])
                                train_mismatch += 1
                                if mismatch_at == -1:
                                    mismatch_at = copy.copy(i)

                        self.assertEqual(train_mismatch, 0,
                                msg="Received spik train is not identical to the sent one.")
                        if train_mismatch > 0:
                            filename = 'mismatched_dump-%d.pkl' % (experiment_no)
                            print "*** dumping to '%s'" % (filename)
                            with open(filename, 'wb') as f:
                                got = []
                                for i in range(received_data.size()):
                                    got.append(received_data[i].getNeuronAddress().value())
                                expected = []
                                for i in range(pc.size()):
                                    expected.append(pc[i].getNeuronAddress().value())

                                dump_data = {
                                    'got': got,
                                    'expected': expected, 
                                }
                                cPickle.dump(dump_data, f, cPickle.HIGHEST_PROTOCOL)

                            print "got | expected"
                            for i in range(mismatch_at-2, mismatch_at+1):
                                print "T: %d, A: %d | T: %d A: %d" % (
                                    received_data[i].getTime(),
                                    received_data[i].getNeuronAddress().value(),
                                    pc[i].getTime(),
                                    pc[i].getNeuronAddress().value()
                                )

                        results.append( (
                            {
                                'random_interval': random_interval, 
                                'isi': isi,
                                'num_pulses': num_pulses,
                                'repetition': repetition,
                            },
                            received_data.size(),
                            train_mismatch
                        ) )

                        # expected pulse address is the configured pulse address with dnc if channel flipped
                        #exp_pa = FPGA.PulseAddress(pulse_address)
                        #exp_pa.setChannel( exp_pa.getChannel().flip(0) )
                        #for i in range(received_data.size()):
                            #rpa = FPGA.PulseAddress(received_data[i])
                            #self.assertEqual( rpa, exp_pa, "Expected {exp}, actual {act}".format(exp=exp_pa, act=rpa))

                        experiment_no += 1

        print "#####################################"
        print "# Results:                          #"
        print "#####################################"
        print
        print "No. | Poisson |   ISI | num_pulses |   received | mismatches | success"

        for no,r in enumerate(results):
            if r[2] == 0 and r[0]['num_pulses'] == r[1]: success = 'pass'
            else: success = 'fail'
            print "%3d |   %5s | %5d | %10d | %10d | %10d | %6s" % (
                no, 
                r[0]['random_interval'],
                r[0]['isi'],
                r[0]['num_pulses'],
                r[1],
                r[2],
                success
            )
             


if __name__ == '__main__':
    HWTest.main()
