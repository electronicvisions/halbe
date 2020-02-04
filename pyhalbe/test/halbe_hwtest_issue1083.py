#!/usr/bin/env python

import numpy as np

from HWTest import HWTest
from pyhalbe import (Coordinate, HICANN, Handle, ADC, FPGA)
from pyhalco_common import top, bottom, left, right, X, Y, Enum


#ishell = lambda *v, **k: None
#try:
#    import sys
#    from IPython.frontend.terminal.embed import InteractiveShellEmbed
#    if sys.stdout.isatty():
#        ishell = InteractiveShellEmbed()
#except ImportError:
#    pass


class ADCHardwareTest(HWTest):
    def setUp(self):
        super(ADCHardwareTest, self).setUp()
        self.adc = Handle.createADCHw()
        self.addCleanup(Handle.freeADCHw, self.adc)

    def get_trace(self, adc_channel, trace_length):
        HICANN.flush(self.h)

        conf = ADC.Config(trace_length,
                          Coordinate.ChannelOnADC(adc_channel),
                          Coordinate.TriggerOnADC(0))
        ADC.config(self.adc, conf)
        ADC.trigger_now(self.adc)

        trace = ADC.get_trace(self.adc)

        v = -0.00066535 * trace + 2.02391
        t = np.arange(trace.size) * 1 / 96e6

        return (trace, v, t)

    def set_denmem_quads(self, neuron, **kwargs):
        nquad = HICANN.NeuronQuad()

        for qq in range(Coordinate.QuadOnHICANN.end):
            HICANN.set_denmem_quad(
                self.h, Coordinate.QuadOnHICANN(qq), nquad)

        # All other neurons keep the default values
        nactive = nquad[neuron.toNeuronOnQuad()]

        for key, value in kwargs.iteritems():
            getattr(nactive, key)(value)

        HICANN.set_denmem_quad(self.h, neuron.toQuadrantOnHICANN(), nquad)

    def set_analog(self, analog, neuron):
        # Configure analog output
        aout = HICANN.Analog()
        aout.enable(analog)

        if (int(neuron.y()) > 0):
            if (int(neuron.x()) % 2):
                aout.set_membrane_bot_odd(analog)
            else:
                aout.set_membrane_bot_even(analog)
        else:
            if (int(neuron.x()) % 2):
                aout.set_membrane_top_odd(analog)
            else:
                aout.set_membrane_top_even(analog)

        HICANN.set_analog(self.h, aout)

    def test_BGToNeuronMembrane(self):
        """Try to send periodic events from a background generator to a neuron.
        Please note that this test relies on the default config values in most objects
        and only changes those necessary to run the test."""

        weight = 15
        period = 700 * 2
        event = HICANN.L1Address(0)
        offevent = HICANN.L1Address(55)

        trace_length = 19500

        exc = True
        inh = False
        E_syn = (570, 570)

        firing = False
        firingevent = HICANN.L1Address(42)

        # FIXME: Differs from HICANN to HICANN, this needs connection database
        analog = Coordinate.AnalogOnHICANN(1)
        adc_channel = 7

        neuron = Coordinate.NeuronOnHICANN(Enum(15))
        vline = Coordinate.VLineOnHICANN(28)  # or 60, 92, 124
        driver = Coordinate.SynapseDriverOnHICANN(Y(111), left)
        synapse_row = Coordinate.SynapseRowOnHICANN(driver, top)

        ##############################
        #  set floating gate values  #
        ##############################

        fgcfg = HICANN.FGConfig()
        fgctrl = HICANN.FGControl()

        # Set same reverse potential for both synaptic inputs (for all neurons)
        for ii in range(512):
            nrn = Coordinate.NeuronOnHICANN(Enum(ii))
            fgctrl.setNeuron(nrn, HICANN.neuron_parameter.E_synx, E_syn[0])
            fgctrl.setNeuron(nrn, HICANN.neuron_parameter.E_syni, E_syn[1])
            fgctrl.setNeuron(nrn, HICANN.neuron_parameter.I_bexp, 0)

        for block in Coordinate.iter_all(Coordinate.FGBlockOnHICANN):
            HICANN.set_fg_config(self.h, block, fgcfg)
        HICANN.set_fg_values(self.h, fgctrl)

        ##############################
        #  set global neuron config  #
        ##############################

        nconf = HICANN.NeuronConfig()
        nconf.bigcap[int(top)] = True
        nconf.bigcap[int(bottom)] = True
        HICANN.set_neuron_config(self.h, nconf)

        #################
        #  set mergers  #
        #################

        dnc = HICANN.DNCMergerLine()
        for i in range(8):
            mer = Coordinate.DNCMergerOnHICANN(i)
            dnc[mer].slow = True
            dnc[mer].config = HICANN.Merger.RIGHT_ONLY

        # Tree defaults to one on one passthrough (forward downwards)
        tree = HICANN.MergerTree()

        HICANN.set_dnc_merger(self.h, dnc)
        HICANN.set_merger_tree(self.h, tree)

        ##########################
        #  background generator  #
        ##########################

        # We use one background generator to provide events
        gens = HICANN.BackgroundGeneratorArray()
        g = gens[7]
        g.enable(True)
        g.random(False)
        g.period(period)
        g.address(event)
        HICANN.set_background_generator(self.h, gens)

        # Forward its output to this HICANN
        srepeater = Coordinate.OutputBufferOnHICANN(7).repeater()

        sr = HICANN.HorizontalRepeater()
        sr.setOutput(right)

        HICANN.set_repeater(self.h, srepeater.horizontal(), sr)

        ###############################################
        #  connect via Crossbars and SynapseSwitches  #
        ###############################################

        hline = srepeater.toHLineOnHICANN()

        cb = HICANN.Crossbar()
        cb.set(vline, hline, True)

        HICANN.set_crossbar_switch_row(self.h, hline, vline.toSideHorizontal(),
                                       cb.get_row(hline, vline.toSideHorizontal()))

        # Connect VLine to synapse driver
        sw = HICANN.SynapseSwitch()
        sw.set(vline, driver.toHLineOnHICANN(), True)

        HICANN.set_syndriver_switch_row(self.h, driver.toSynapseSwitchRowOnHICANN(),
                                        sw.get_row(driver.toSynapseSwitchRowOnHICANN()))

        ########################
        #  set synapse driver  #
        ########################

        drv = HICANN.SynapseDriver()
        drv.set_l1()
        drv[top].set_syn_in(left, exc)
        drv[top].set_syn_in(right, inh)
        drv[bottom].set_syn_in(left, exc)
        drv[bottom].set_syn_in(right, inh)

        HICANN.set_synapse_driver(self.h, HICANN.SynapseController(), driver, drv)

        decoders = HICANN.DecoderDoubleRow()
        weights = HICANN.WeightRow()

        # Reset decoders and weights to 'off' state
        for ii in range(2):
            for jj in range(256):
                decoders[ii][jj] = offevent.getSynapseDecoderMask()

        for ii in range(256):
            weights[ii] = 0

        # Only enable decoders/weights for background generator and neuron
        decoders[0][int(neuron.x())] = event.getSynapseDecoderMask()
        weights[int(neuron.x())] = weight

        HICANN.set_decoder_double_row(self.h, HICANN.SynapseController() ,driver, decoders)
        HICANN.set_weights_row(self.h, HICANN.SynapseController(), synapse_row, weights)

        ###################
        #  get ADC trace  #
        ###################

        self.set_denmem_quads(
            neuron, address=firingevent,
            activate_firing=firing,
            enable_aout=True)
        self.set_analog(analog, neuron)
        trace, v, t = self.get_trace(adc_channel, trace_length)

        if False:
            import pylab
            fig, ax = pylab.subplots()
            ax.plot(t, v)
            pylab.show()

        ## Case 1: Sometimes the membrane seems 'stuck' at 1.2V
        self.assertNotAlmostEqual(v.mean(), 1.2, places=1)
        self.assertGreater(v.std(), 0.01)

        ## TODO: check for periodic peaks using fourier magic

    def test_SweepNeurons(self):
        """When the exponential term of a neuron is disabled and firing is off
        V_t should not have any effect on the rest potential."""

        # FIXME: Differs from HICANN to HICANN, this needs connection database
        analog = Coordinate.AnalogOnHICANN(0)
        adc_channel = 3
        bigcap = True
        trace_length = 1950

        neurons = [Coordinate.NeuronOnHICANN(Enum(ii))
                   for ii in range(512)]

        threshold_voltages = np.arange(0, 1024, step=20, dtype=np.ushort)
        membrane = np.zeros((len(neurons), threshold_voltages.size))

        nconf = HICANN.NeuronConfig()
        nconf.bigcap[int(top)] = bigcap
        nconf.bigcap[int(bottom)] = bigcap
        HICANN.set_neuron_config(self.h, nconf)

        for ii, V_t in enumerate(threshold_voltages):

            fgctrl = HICANN.FGControl()

            for nrn in neurons:
                fgctrl.setNeuron(nrn, HICANN.neuron_parameter.V_t, V_t)
                fgctrl.setNeuron(nrn, HICANN.neuron_parameter.I_bexp, 0)

            for block in [Coordinate.FGBlockOnHICANN(Enum(xx)) for xx in range(4)]:
                HICANN.set_fg_values(self.h, block, fgctrl.getBlock(block))

            for jj, neuron in enumerate(neurons):
                self.set_denmem_quads(neuron, enable_aout=True)
                self.set_analog(analog, neuron)

                trace, v, t = self.get_trace(
                    adc_channel, trace_length)
                membrane[jj, ii] = v.mean()

        if False:
            import pylab
            import matplotlib

            fig, ax = pylab.subplots()
            cax = ax.imshow(membrane.T, interpolation='nearest', cmap=matplotlib.cm.coolwarm)
            fig.colorbar(cax, orientation='horizontal')
            fig.show()

        # Case 1: Radically different behaviour between upper neurons and lower neurons.
        std = membrane.std(axis=1)
        self.assertAlmostEqual(std[:256].mean(), std[256:].mean(), places=2)

        # Case 2: Ideally we would want uniform behaviour for all values of V_t.
        self.assertLess(membrane.mean(axis=0).std(), 0.05)


if __name__ == '__main__':
    ADCHardwareTest.main()
