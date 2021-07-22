#!/usr/bin/env python

import argparse
import pyhalbe


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--ip", action="store", type=str, required=True,
                        help="FPGA IP, e.g. 192.168.1.7")
    parser.add_argument("--fpga", action="store", type=int, required=True,
                        help="FPGA coordinate, 0 on vertical setup")
    parser.add_argument("--dnc", action="store", type=int, required=True,
                        help="DNCOnFPGA coordinate, 1 on vertical setup")
    args = parser.parse_args()

    ip = pyhalbe.Coordinate.IPv4.from_string(args.ip)
    handle = pyhalbe.Handle.FPGAHw(pyhalbe.Coordinate.FPGAGlobal(args.fpga), ip, pyhalbe.Coordinate.DNCOnFPGA(args.dnc))
    status = pyhalbe.FPGA.get_fpga_status(handle)

    print(("FPGA design revision: {}".format(status.design_version)))
