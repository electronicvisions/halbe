#!/usr/bin/env python
import unittest
from HWTest import HWTest
from pyhalbe import *

class FPGAStatusHWTests(HWTest):
    def test_DesignVersion(self):
        design_version = FPGA.get_fpga_status(self.fpga).getDesignVersion()
        self.assertGreater(design_version, 900)

if __name__ == '__main__':
    FPGAStatusHWTests.main()
