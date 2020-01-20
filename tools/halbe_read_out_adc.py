#!/usr/bin/env python

import pyhalco_hicann_v2 as Coordinate
from pyhalbe import ADC, Handle
import argparse
import pylogging
import time
import numpy as np

def measure(adc, ip, port, channel, samples):
    handle = None
    if ip:
        assert port
        ip = Coordinate.IPv4.from_string(ip)
        port = Coordinate.TCPPort(port)
        handle = Handle.ADCRemoteHw(ip, port, adc)
    else: # local handle
        handle = Handle.ADCHw(adc)
    config = ADC.Config(
            samples,
            channel,
            Coordinate.TriggerOnADC(0))
    ADC.config(handle, config)
    dt = 1.0/ADC.get_sample_rate(handle)
    ADC.trigger_now(handle)
    time.sleep(dt * samples)
    trace = ADC.get_trace(handle)
    #FIXME:
    #Handle.freeADCHw(handle)
    t = np.arange(len(trace))*dt
    return np.array([t, trace])

def dump(data):
    for t, v in data.T:
        print "{}\t{}".format(t,v)

def plot(data):
    from matplotlib import pyplot as plt
    plt.plot(data[0], data[1], '.')
    plt.show()

def init_logger():
    layout = pylogging.ColorLayout(True)
    appender = pylogging.ConsoleAppender(layout)
    appender.setOption("target", pylogging.ConsoleAppender.getSystemErr())
    appender.activateOptions(pylogging.Pool())

    l = pylogging.get_root()
    pylogging.set_loglevel(l, pylogging.LogLevel.DEBUG)
    l.addAppender(appender)

def main():
    parser = argparse.ArgumentParser(description = 'Reads (and plots) ADC trace')
    parser.add_argument('--adc', type=Coordinate.ADC, help='ADC Board Id', required=True)
    parser.add_argument('--ip', type=str, help='ADC Service IP', required=False)
    parser.add_argument('--port', type=int, help='ADC Service Port', required=False)
    parser.add_argument('--channel', type=lambda x: Coordinate.ChannelOnADC(int(x)), help="MUX",
            required=False, default=Coordinate.ChannelOnADC(0))
    parser.add_argument('--samples', type=int, help='Number of samples to collect', required=True)
    parser.add_argument('--calibration', action='store_true',
            help='Try to apply calibration')
    parser.add_argument('--dump', action='store_true',
            help='Dump trace data to console')
    parser.add_argument('--plot', action='store_true',
            help='Plot trace data using matplotlib')
    args = parser.parse_args()

    init_logger()

    data = measure(args.adc, args.ip, args.port, args.channel, args.samples)

    if args.dump:
        dump(data)
    if args.plot:
        plot(data)

if __name__ == '__main__':
    main()
