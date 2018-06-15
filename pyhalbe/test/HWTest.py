#!/usr/bin/env python

import sys
import unittest
from Test import PyhalbeTest, parametrize

class HWTest(PyhalbeTest):
    def setUp(self):
        if 'nose' in sys.modules.keys():
            # ugly hack to support nose-based execution...
            self.FPGA_IP = '0.0.0.0'
            self.PMU_IP = '0.0.0.0'
            self.HICANN   = 0
            self.DNC      = 1
            self.FPGA     = 0
            self.LOGLEVEL = 2
            self.ON_WAFER = False
            self.WAFER  = 0
            self.USE_SCHERIFF = False

        from pyhalbe import Handle, Coordinate, HICANN, FPGA, Coordinate, Debug
        # The module pyhalbe.apicheck wraps pyhalbe for hardware-less
        # apichecks. It will be enabled only if the environment variable
        # PYHALBE_API_CHECK is set to true. The attribute "enabled" will be set
        # accordingly. KHS.
        import pyhalbe_apicheck as apicheck

        Enum = Coordinate.Enum
        highspeed = True
        arq = True
        hicann_num = 1
        fpga_ip = Coordinate.IPv4.from_string(self.FPGA_IP)
        pmu_ip = Coordinate.IPv4.from_string(self.pmu_IP)
        if self.LOGLEVEL >= 0:
            Debug.change_loglevel(self.LOGLEVEL)

        self.dnc = Coordinate.DNCOnFPGA(Enum(self.DNC))
        self.hicann = Coordinate.HICANNOnDNC(Enum(self.HICANN))
        self.f = Coordinate.FPGAGlobal(Enum(self.FPGA), Coordinate.Wafer(Enum(self.WAFER)))

        self.fpga = Handle.createFPGAHw(self.f, fpga_ip, self.dnc, self.ON_WAFER, hicann_num, self.KINTEX, pmu_ip)
        if self.USE_SCHERIFF:
            self.fpga.enableScheriff()
        self.addCleanup(Handle.freeFPGAHw, self.fpga)

        self.h  = self.fpga.get(self.dnc, self.hicann)
        if self.ON_WAFER:
            self.h0 = self.fpga.get(self.dnc, Coordinate.HICANNOnDNC(Enum(0)))
            self.h1 = self.fpga.get(self.dnc, Coordinate.HICANNOnDNC(Enum(1)))
            self.h2 = self.fpga.get(self.dnc, Coordinate.HICANNOnDNC(Enum(2)))
            self.h3 = self.fpga.get(self.dnc, Coordinate.HICANNOnDNC(Enum(3)))
            self.h4 = self.fpga.get(self.dnc, Coordinate.HICANNOnDNC(Enum(4)))
            self.h5 = self.fpga.get(self.dnc, Coordinate.HICANNOnDNC(Enum(5)))
            self.h6 = self.fpga.get(self.dnc, Coordinate.HICANNOnDNC(Enum(6)))
            self.h7 = self.fpga.get(self.dnc, Coordinate.HICANNOnDNC(Enum(7)))


        self.apicheck = apicheck.enabled()
        if apicheck.enabled(): # equals PYHALBE_API_CHECK
            # With hardware access disabled pyhalbe functions return anything
            # but the expected, i.e. all unittest assertions naturally fail.
            # I.e. for a functional API check they have to be disabled. KHS.
            apicheck.monkeyPatch(self, 'assert') # disables all functions in self that start with "assert"

        # ECM says: this should be part of the tests... automatic resets during construction isn't very "standalone"
        # OR it is necessary and in this case it should be a member function and documented somehow.
        # FPGA reset
        FPGA.reset(self.fpga)
        HICANN.init(self.h, False)
        if self.ON_WAFER:
            HICANN.init(self.h0, False)
            HICANN.init(self.h1, False)
            HICANN.init(self.h2, False)
            HICANN.init(self.h3, False)
            HICANN.init(self.h4, False)
            HICANN.init(self.h5, False)
            HICANN.init(self.h6, False)
            HICANN.init(self.h7, False)


    @classmethod
    def main(self, *was_args, **was_kwargs):
        import argparse
        import sys

        def jtag_type(s):
            hwbes = {
                'w': 'WAFER',
                'v': 'VERTICAL'
            }
            try:
                return hwbes[s.lower()[0]]
            except KeyError:
                raise Exception('Unsupported hardware backend. Please select from: %s' % str(hwbes))

        parser = argparse.ArgumentParser(description='HWTest: %s' % self.__name__)
        parser.add_argument('--fpga_ip',   action='store', required = True,
                type=str,
                help='specify FPGA ip')
        parser.add_argument('--mauc_ip',   action='store', required = True,
                type=str,
                help='specify FPGA ip')
        parser.add_argument('--on', action='store', required = False,
                help='specify hardware backend [[w]afer,[v]ertical]', type=jtag_type)
        parser.add_argument('--h', '--hicann', action='store', required = False,
                type=int, default=0, dest='h',
                help='specify HICANN (FPGA-local enum)')
        parser.add_argument('--d', '--dnc', action='store', required = False,
                type=int, default=1, dest='d',
                help='specify DNC (FPGA-local enum)')
        parser.add_argument('--f', '--fpga', action='store', required = False,
                type=int, default=0, dest='f',
                help='specify FPGA (global enum)')
        parser.add_argument('--w', '--wafer', action='store', required = False,
                type=int, default=0, dest='w',
                help='specify Wafer (global enum)')
        parser.add_argument('--kintex', action='store', required = False,
                type=int, default=0, dest='kintex',
                help='Kintex mode (otherwise Virtex)?')
        parser.add_argument('--loglevel', action='store', required = False,
                type=int, default=-1,
                help='specify loglevel [0-ERROR,1-WARNING(default),2-INFO,3-DEBUG0,4-DEBUG1,5-DEBUG2,6-DEBUG3]')
        parser.add_argument('--kill-scheriff', action='store_true', help='disable scheriff')
        parser.add_argument('--xml-output-dir', action='store', default=None,
                required=False, help='create xml reports')

        args, argv = parser.parse_known_args()
        argv.insert(0, sys.argv[0])

        HWTest.FPGA_IP  = args.fpga_ip
        HWTest.PMU_IP  = args.pmu_ip
        HWTest.HICANN   = args.h
        HWTest.DNC      = args.d
        HWTest.FPGA     = args.f
        HWTest.LOGLEVEL = args.loglevel
        HWTest.ON_WAFER = (args.on == 'WAFER') or args.w > 0
        HWTest.WAFER    = args.w
        HWTest.USE_SCHERIFF = not args.kill_scheriff
        HWTest.KINTEX   = args.kintex
        test_runner = None
        if args.xml_output_dir:
            from xmlrunner import XMLTestRunner
            test_runner = XMLTestRunner(output=args.xml_output_dir)
        unittest.main(testRunner=test_runner, argv=argv, *was_args, **was_kwargs)

if __name__ == '__main__':
    HWTest.main()
