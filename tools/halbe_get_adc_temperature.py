#!/usr/bin/env python

import pyhalbe


def get_temperature(adc_id):
    c_adc = pyhalbe.Coordinate.ADC(adc_id)
    h_adc = pyhalbe.Handle.ADCHw(c_adc)
    temperature = pyhalbe.ADC.get_temperature(h_adc)
    pyhalbe.Handle.freeADCHw(h_adc)

    return temperature


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--adc", action="store", type=str, required=True,
                        help="ADC identifier")
    args = parser.parse_args()
    print("ADC temperature: {:.2f} deg C".format(get_temperature(args.adc)))
