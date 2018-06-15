#!/usr/bin/env python

import pyhalbe


def get_rev(adc_id):
    c_adc = pyhalbe.Coordinate.ADC(adc_id)
    h_adc = pyhalbe.Handle.ADCHw(c_adc)
    status = pyhalbe.ADC.get_status(h_adc)
    pyhalbe.Handle.freeADCHw(h_adc)

    return status.version_string


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--adc", action="store", type=str, required=True,
                        help="ADC identifier")
    args = parser.parse_args()
    print("ADC FPGA design revision: {}".format(get_rev(args.adc)))
