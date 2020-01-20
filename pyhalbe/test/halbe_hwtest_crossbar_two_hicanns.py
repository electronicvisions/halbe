#!/usr/bin/env python
import unittest
from pyhalbe import *
from pyhalco_common import Enum, left, right
import pyhalco_hicann_v2 as Coordinate

class TwoHicannTest(unittest.TestCase):
    """
    Test with 2 HICANNs on the vertical setup
    """
    def setUp(self):
        if not hasattr(self, 'ip'):
            raise unittest.SkipTest("no hardware")

    def test_CrossbarWriteRead(self):
        highspeed = True
        hicann_num = 2
        ip = self.ip
        port = 1701

        myPowerBackend = PowerBackend.instance(PowerBackend.VerticalSetup)
        myPowerBackend.SetupReticle(ip, port, hicann_num, highspeed, True)
        h1 = Handle.HICANN(Coordinate.HICANNGlobal(Enum(0)))
        h2 = Handle.HICANN(Coordinate.HICANNGlobal(Enum(1)))
        init(h1)
        init(h2)
        import numpy.random as rd
        def create_pattern():
            return [[ bool(i < 0.5) for i in rd.randint(2, size=4)] for n in xrange(64)]
        patterns_left = [ create_pattern() for n in xrange(2) ]
        patterns_right = [ create_pattern() for n in xrange(2) ]

        # UGLY!
        read_pattern = [[True]*4]*64


        for n,h in enumerate([h1,h2]):
            for i in range(0, 64):
                set_crossbar_switch_row(h, Coordinate.HLineOnHICANN(i), left, patterns_left[n][i])
                set_crossbar_switch_row(h, Coordinate.HLineOnHICANN(i), right, patterns_right[n][i])
        for n,h in enumerate([h1,h2]):
            # left
            for i in range(0, 64):
                read_pattern[i] = get_crossbar_switch_row(h, Coordinate.HLineOnHICANN(i), left)
            self.assertEqual(patterns_left[n], read_pattern)

            # right
            for i in range(0, 64):
                read_pattern[i] = get_crossbar_switch_row(h, Coordinate.HLineOnHICANN(i), right)
            self.assertEqual(patterns_right[n], read_pattern)

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument('--ip', action='store', type=str, required=True)
    args = parser.parse_args()

    TwoHicannTest.ip = Coordinate.IPv4.from_string(args.ip)

    unittest.main()
