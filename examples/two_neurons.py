"""
    Author: Sebastian Jeltsch <sjeltsch@kip.uni-heidelberg.de>

    Two Neuron Network DEMO

    There are two neurons {0,1}. The first is stimulated by perdiodic
    background generator input and the second is stimulated by the first.
    Each neuron is mapped to one of the analog outputs.
"""

import argparse
import pyoneer
from pyhalbe import *
from pyhalbe.HICANN import *
from pyhalco_hicann_v2 import left as LEFT, right as RIGHT
from pyhalco_hicann_v2 import top as TOP, bottom as BOTTOM
from pyhalco_hicann_v2 import Enum, Y, X, IPv4, HICANNOnFPGA, DNCOnFPGA, FGBlockOnHICANN, \
    DNCMergerOnHICANN, NeuronOnHICANN, AnalogOnHICANN, RepeaterBlockOnHICANN, \
    SendingRepeaterOnHICANN, VLineOnHICANN, SynapseSwitchRowOnHICANN, SynapseRowOnHICANN

def main(args):

    ip = IPv4.from_string(args.ip)
    on_wafer = False

    dnc    = DNCOnFPGA(Enum(args.dnc))
    f = Coordinate.FPGAGlobal(Enum(0))
    fpga   = Handle.FPGA(f, ip, dnc, on_wafer)
    hicann = fpga.get(dnc, HICANNOnDNC(Enum(args.hicann)))

    reset(hicann)
    init(hicann, True)

    # write FG Blocks first, before any topology
    set_fg_values(hicann, FGControl())

    # configure backgroud generator 7 for stimulation of Neuron0
    bg = BackgroundGeneratorArray()
    bg[7].enable(True)
    bg[7].random(False)
    bg[7].seed(200)
    bg[7].period(1500)
    bg[7].address(L1Address(0))
    set_background_generator(hicann, bg)

    # write default Neuron config
    set_neuron_config(hicann, NeuronConfig())

    neurons = [
            ( NeuronOnHICANN(Enum(0)), L1Address( 0) ),
            ( NeuronOnHICANN(Enum(1)), L1Address(32) ),
            ]

    nquad = NeuronQuad()
    for nrn, addr in neurons:
        neu = nquad[nrn.toNeuronOnQuad()]
        neu.address(addr)
        neu.activate_firing(True)
        neu.enable_spl1_output(True)
        neu.enable_aout(True)

    set_denmem_quad(hicann, neurons[0][0].toQuadOnHICANN(), nquad)


    # config analog outputs to observe neurons
    aout = Analog()
    for idx, nrn in enumerate(neurons):
        aout.enable(AnalogOnHICANN(idx))
        if int(nrn[0].x()) % 2 == 0: #even
            aout.set_membrane_top_even(AnalogOnHICANN(idx))
        else:
            aout.set_membrane_top_odd(AnalogOnHICANN(idx))

    set_analog(hicann, aout)


    # L1 Topology

    dncmerger = DNCMergerLine()
    for mm in map(DNCMergerOnHICANN, range(8)):
        dncmerger[mm].config = DNCMerger.RIGHT_ONLY
    set_dnc_merger(hicann, dncmerger)

    set_merger_tree(hicann, MergerTree())
    set_phase(hicann, 0)

    # write default RepeatBlocks
    for ii in map(RepeaterBlockOnHICANN, map(Enum, range(6))):
        set_repeater_block(hicann, ii, RepeaterBlock())

    cb = Crossbar()
    sw = SynapseSwitch()

    paths = [ (SendingRepeaterOnHICANN(0), VLineOnHICANN(28), SynapseSwitchRowOnHICANN(Y(111), LEFT), 0), # BG7 -> neuron0
            (SendingRepeaterOnHICANN(7), VLineOnHICANN( 0), SynapseSwitchRowOnHICANN(Y( 97), LEFT), 1) ]  # neuron0 -> neuron1

    for repeater, vline, row, target_nrn in paths:

        sr = HorizontalRepeater()
        sr.setOutput(RIGHT)
        set_repeater(hicann, repeater, sr);

        # crossbar switches
        cb.set(vline, repeater.line(), True);
        switches = cb.get_row(repeater.line(), row.toSideHorizontal())
        set_crossbar_switch_row(hicann, repeater.line(), row.toSideHorizontal(), switches)

        # synapse driver switches
        sw.set(vline, row.line(), True);
        set_syndriver_switch_row(hicann, row, sw.get_row(row))

        # Synapse Driver
        driver = SynapseDriver()
        driver.set_l1()

        rconfig = RowConfig()
        rconfig.set_gmax_div(LEFT, 1)
        rconfig.set_gmax_div(RIGHT, 1)
        rconfig.set_syn_in(LEFT, 1)
        driver[TOP] = driver[BOTTOM] = rconfig

        set_synapse_driver(hicann, row.toSynapseDriverOnHICANN(), driver);

        # Synapses

        nrn = int(neurons[target_nrn][0].x())

        # synapse decoders
        decoders = DecoderDoubleRow()
        for ii in range(2):
            for jj in range(256):
                decoders[ii][jj] = 0xf # blocks L1Addres(0)
            decoders[ii][nrn] = 0

        set_decoder_double_row(hicann, row.toSynapseDriverOnHICANN(), decoders)

        # synapse weights
        weights = WeightRow()
        weights[nrn] = 15

        set_weights_row(hicann, SynapseRowOnHICANN(row.toSynapseDriverOnHICANN(), TOP), weights);
        set_weights_row(hicann, SynapseRowOnHICANN(row.toSynapseDriverOnHICANN(), BOTTOM), weights);


if __name__ == '__main__':

    parser = argparse.ArgumentParser()

    add_arg = lambda arg, type, **kwargs : parser.add_argument(
            '--%s' % arg, action='store', type=type, **kwargs)

    add_arg('ip', str, required=True)
    add_arg('port', int, default=1701)
    add_arg('hicann', int, default=0)
    add_arg('dnc', int, default=1)
    add_arg('fpga', int, default=0)

    add_arg('hw', bool, default=False, help='use hardware')
    add_arg('exp', int, default=-1, help='experiment ID')

    args = parser.parse_args()

    import pyoneer
    pyo = Handle.get_pyoneer()
    pyo.useHardware = args.hw
    pyo.experiment.id = args.exp

    main(args)

    print "experiment id: %d" % pyo.experiment.id
