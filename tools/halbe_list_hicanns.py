#!/usr/bin/env python

import sys
import argparse
import pylogging
from halbe_convert_coordinate import add_coordinate
import Coordinate as C

WAFER = C.Wafer()

def f_wafer(coord):
    global WAFER
    WAFER = coord
    return []

def f_fpga(coord):
    gcoord = C.FPGAGlobal(coord, WAFER)
    result = []
    for dnc_f in C.iter_all(C.DNCOnFPGA):
        dnc = dnc_f.toDNCOnWafer(gcoord)
        result += hicanns_on_dnc(dnc)
    return result

def f_dnc(dnc):
    gdnc = C.DNCGlobal(dnc, WAFER)
    return hicanns_on_dnc(gdnc)

def f_reticle(coord):
    dnc = coord.toDNCOnWafer()
    gdnc = C.DNCGlobal(dnc, WAFER)
    return hicanns_on_dnc(gdnc)

def hicanns_on_dnc(dnc):
    per_dnc = C.HICANNOnDNC.enum_type.size
    offset = (per_dnc / 2 - 1)

    h0 = C.HICANNOnDNC(C.Enum(0)).toHICANNOnWafer(dnc)
    h1 = C.HICANNOnDNC(C.Enum(per_dnc / 2)).toHICANNOnWafer(dnc)

    top = range(h0.id().value(), h0.id().value() + offset + 1)
    bot = range(h1.id().value(), h1.id().value() + offset + 1)
    return top + bot

def main():
    description = 'Lists HICANNs on a reticle/dnc/FPGA'
    epilog = "example: {} --fpga 0".format(sys.argv[0])

    parser = argparse.ArgumentParser(description=description, epilog=epilog)
    add_coordinate(parser, "--wafer", C.Wafer, f_wafer)
    parser.parse_known_args()  # Ensure global wafer is set
    add_coordinate(parser, "--fpga", C.FPGAOnWafer, f_fpga)
    add_coordinate(parser, "--dnc", C.DNCOnWafer, f_dnc)
    add_coordinate(parser, "--power", C.PowerCoordinate, f_reticle)
    args = parser.parse_args(namespace=argparse.Namespace(values=[]))

    # The arguments return a list of lists of lists
    values = sum(sum(args.values, []), [])
    if not values:
        parser.error("No HICANNs found, not enough arguments passed.")
    for value in values:
        print value

if __name__ == '__main__':

    pylogging.reset()
    pylogging.default_config(date_format='absolute')

    main()
