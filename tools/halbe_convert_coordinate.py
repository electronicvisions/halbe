#!/usr/bin/env python
"""
Tool to print nice informations about coodinates

usage example: ./convert_coordinate.py --hicann 0 --hicann 5,5 --reticle 1
"""

import argparse
import sys
import pyhalco_hicann_v2 as Coordinate
import pylogging
from pyhalco_common import X, Y, Enum, iter_all

WAFER = Coordinate.Wafer(10) # Show some new wafer as default

def add_coordinate(parser, arg_name, CoordinateType, formatter):
    if hasattr(CoordinateType, "x_type") and hasattr(CoordinateType, "y_type"):
        def parse_arg(arg):
            tmp = arg.split(",")
            if len(tmp) == 1:
                coord = CoordinateType(Enum(int(tmp[0])))
            elif len(tmp) == 2:
                x, y = tmp
                coord = CoordinateType(X(int(x)), Y(int(y)))
            else:
                raise argparse.ArgumentTypeError(
                        "Please provide {0} <x,y> or {0} <enum>".format(
                            arg_name))
            return formatter(coord)
        metavar = '<enum>|<x>,<y>'
    else:
        def parse_arg(arg):
            coord = CoordinateType(Enum(int(arg)))
            return formatter(coord)
        metavar = '<int>'

    coordinate_name = CoordinateType.__name__
    parser.add_argument(arg_name, nargs='*', action='append',
            type=parse_arg, metavar=metavar, dest='values',
            help='print coordinates related to {}'.format(coordinate_name))

def f_wafer(coord):
    global WAFER
    WAFER = coord

def print_hicann(coord):
    return "{:<27} (id: {:>3})):\n".format(coord, coord.toEnum().value())

def f_hicann(hicann):
    """prints Hicann related variables"""
    ghicann = Coordinate.HICANNGlobal(hicann, WAFER)
    out = print_hicann(ghicann)
    fpga = ghicann.toFPGAOnWafer()
    dnc = ghicann.toDNCGlobal()
    for coordinate in [fpga, dnc, dnc.toDNCOnFPGA(),
                       dnc.toPowerCoordinate(), hicann.toHICANNOnDNC()]:
        has_id = getattr(coordinate, "id", None)
        if has_id:
            out += "\t{} ({})\n".format(coordinate, coordinate.toEnum())
        else:
            out += "\t{}\n".format(coordinate)
    try:
        import pysthal
        db = pysthal.MagicHardwareDatabase()
        ip = db.get_fpga_ip(Coordinate.HICANNGlobal(hicann, WAFER))
        port = 1700 + dnc.toDNCOnFPGA().value()
        out += "\t{} {}".format(ip.to_string(), port)
    except Exception:
        pass
    return out

def print_hicanns_on_dnc(dnc, indent=""):
    per_dnc = Coordinate.HICANNOnDNC.enum_type.size
    offset = (per_dnc / 2 - 1)
    h0 = Coordinate.HICANNOnDNC(Enum(0)).toHICANNOnWafer(dnc)
    h1 = Coordinate.HICANNOnDNC(Enum(per_dnc / 2)).toHICANNOnWafer(dnc)
    hid0 = h0.toEnum()
    hid1 = h1.toEnum()
    return "{}{} - {} ({} - {}, {} - {})\n".format(indent, h0, h1,
            hid0, Enum(hid0.value() + offset),
            hid1, Enum(hid1.value() + offset))

def f_fpga(coord):
    """print fpga related infomations"""
    gcoord = Coordinate.FPGAGlobal(coord, WAFER)
    out = "{}:\n".format(gcoord)
    for dnc_f in iter_all(Coordinate.DNCOnFPGA):
        dnc = dnc_f.toDNCOnWafer(gcoord)
        out += "\t{} ({}, {}):\n".format(dnc, dnc.toEnum(), dnc.toPowerCoordinate())
        out += print_hicanns_on_dnc(dnc, "\t\t")
    return out

def f_dnc(dnc):
    """print fpga related infomations"""
    gdnc = Coordinate.DNCGlobal(dnc, WAFER)
    out = "{} ({}):\n".format(gdnc, gdnc.toEnum())
    out += "\t{}\n".format(gdnc.toFPGAOnWafer())
    out += "\t{}\n".format(gdnc.toPowerCoordinate())
    out += print_hicanns_on_dnc(gdnc, "\t")
    return out

def f_reticle(coord):
    """print fpga related infomations"""
    dnc = coord.toDNCOnWafer()
    gdnc = Coordinate.DNCGlobal(dnc, WAFER)
    out = "{}:\n".format(coord)
    out += "\t{}\n".format(gdnc.toFPGAOnWafer())
    out += "\t{} ({})\n".format(dnc, dnc.toEnum())
    out += print_hicanns_on_dnc(dnc, "\t")
    return out

def main():
    """main"""

    description = 'Shows information about HALbe coodinates'
    epilog = "example: {} --hicann 0 --hicann 5,5 --fpga 0 --power 1".format(
            sys.argv[0])

    parser = argparse.ArgumentParser(description=description, epilog=epilog)
    add_coordinate(parser, "--wafer", Coordinate.Wafer, f_wafer)
    add_coordinate(parser, "--hicann", Coordinate.HICANNOnWafer, f_hicann)
    add_coordinate(parser, "--fpga", Coordinate.FPGAOnWafer, f_fpga)
    add_coordinate(parser, "--dnc", Coordinate.DNCOnWafer, f_dnc)
    add_coordinate(parser, "--power", Coordinate.PowerCoordinate, f_reticle)
    args = parser.parse_args()

    if args.values:
        for (value, ) in args.values:
            if value:
                print(value)
    else:
        parser.print_usage()

if __name__ == '__main__':

    pylogging.reset()
    pylogging.default_config(date_format='absolute')

    main()
