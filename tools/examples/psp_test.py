#!/usr/bin/env python
# -*- coding: utf-8 -*-

# In this example we stimulate a neuron using an external spike train in order
# to generate PSPs.
# Plot the voltage traces and spikes using
#     $ python psp_test_plot.py

import os

import numpy as np

import pyhalbe
import pysthal

# Set up shortcuts
Coordinate = pyhalbe.Coordinate
HICANN = pyhalbe.HICANN

# Choose coordinates
NRN = 0

wafer_c = Coordinate.Wafer(0)
hicann_c = Coordinate.HICANNOnWafer(Coordinate.Enum(84))
neuron_c = Coordinate.NeuronOnHICANN(Coordinate.Enum(NRN))

import argparse
parser = argparse.ArgumentParser()
parser.add_argument("in_addr", type=int)
parser.add_argument("vertical", type=str, choices=["top", "bottom"])
args = parser.parse_args()

bg_addr = pyhalbe.HICANN.L1Address(0)
in_addr_int = args.in_addr
in_addr = pyhalbe.HICANN.L1Address(in_addr_int)

# Get wafer and HICANN
wafer = pysthal.Wafer(wafer_c)
hicann = wafer[hicann_c]

neuron = hicann.neurons[neuron_c]
neuron.activate_firing(True)

# Configure background generators to fire on L1 address 0
for bg in Coordinate.iter_all(Coordinate.BackgroundGeneratorOnHICANN):
    generator = hicann.layer1[bg]
    generator.enable(True)
    generator.random(False)
    generator.period(200) # 16 bits!
    generator.address(bg_addr)

# Reconfigure merger tree. By default it gets routed from top to bottom (one-to-one).
#hicann.layer1[Coordinate.Merger0OnHICANN(0)].config = HICANN.Merger.LEFT_ONLY

# Configure DNC mergers.:
for merger in Coordinate.iter_all(Coordinate.DNCMergerOnHICANN):
    hicann.layer1[merger].slow = True # Needed for sending repeaters

# Configure sending repeater to forward spikes to the right
h_line = Coordinate.HLineOnHICANN(62)
sending_repeater = hicann.repeater[Coordinate.HRepeaterOnHICANN(h_line, Coordinate.left)]
sending_repeater.setOutput(Coordinate.right, True)

# Enable a crossbar switch to route the signal into the first vertical line
v_line_c = Coordinate.VLineOnHICANN(0)
hicann.crossbar_switches.set(v_line_c, h_line, True)

# Configure synapse switches and forward to synapse switch row 81
driver_line_c = Coordinate.SynapseSwitchRowOnHICANN(Coordinate.Y(81), Coordinate.left)
hicann.synapse_switches.set(v_line_c, driver_line_c.line(), True)

# Configure synapse driver
driver = hicann.synapses[Coordinate.SynapseDriverOnHICANN(driver_line_c)]

driver.set_l1() # Set to process spikes from L1

driver[Coordinate.top].set_gmax_div(Coordinate.left, 11) # Set 'base' weight
driver[Coordinate.top].set_gmax_div(Coordinate.right, 11)
driver[Coordinate.top].set_syn_in(Coordinate.left, 1) # Forward to left neuron input
driver[Coordinate.top].set_syn_in(Coordinate.right, 0) # … and do not to the right one!
driver[Coordinate.top].set_gmax(0) # Select floating gate value (4 different different strenghts available/analog values)

driver[Coordinate.bottom] = driver[Coordinate.top] # Copy settings from top row to bottom one

if args.vertical == "top":
    driver[Coordinate.top].set_decoder(Coordinate.top, in_addr.getDriverDecoderMask())
    driver[Coordinate.top].set_decoder(Coordinate.bottom, 0)
    driver[Coordinate.bottom].set_decoder(Coordinate.top, 0)
    driver[Coordinate.bottom].set_decoder(Coordinate.bottom, 0)
else:
    driver[Coordinate.top].set_decoder(Coordinate.top, 0)
    driver[Coordinate.top].set_decoder(Coordinate.bottom, 0)
    driver[Coordinate.bottom].set_decoder(Coordinate.top, in_addr.getDriverDecoderMask())
    driver[Coordinate.bottom].set_decoder(Coordinate.bottom, 0)

# Configure synaptic inputs
synapse_line_top_c = Coordinate.SynapseRowOnHICANN(Coordinate.SynapseDriverOnHICANN(driver_line_c.line(), Coordinate.left), Coordinate.top)
synapse_line_bottom_c = Coordinate.SynapseRowOnHICANN(Coordinate.SynapseDriverOnHICANN(driver_line_c.line(), Coordinate.left), Coordinate.bottom)

synapse_line_top = hicann.synapses[synapse_line_top_c]
synapse_line_bottom = hicann.synapses[synapse_line_bottom_c]

# disable
for n in range(0,256):
    synapse_line_top.weights[n] = HICANN.SynapseWeight(0)
    synapse_line_bottom.weights[n] = HICANN.SynapseWeight(0)
    synapse_line_top.decoders[n] = HICANN.SynapseDecoder(1)
    synapse_line_bottom.decoders[n] = HICANN.SynapseDecoder(1)

# … for first neuron
synapse_line_top.weights[NRN] = HICANN.SynapseWeight(15 if args.vertical == "top" else 0) # Set weights
synapse_line_bottom.weights[NRN] = HICANN.SynapseWeight(15 if args.vertical == "bottom" else 0)
if args.vertical == "top":
    synapse_line_top.decoders[NRN] = HICANN.SynapseDecoder(in_addr.getSynapseDecoderMask()) # And listen for spikes from address in_addr
else:
    synapse_line_bottom.decoders[NRN] = HICANN.SynapseDecoder(in_addr.getSynapseDecoderMask())

# Now we can set the floating gate parameters for the neurons
fg = hicann.floating_gates

# First neuron

if False:
    fg_block = neuron_c.toSharedFGBlockOnHICANN()
    fg.setShared(fg_block, HICANN.V_gmax0, 50)
    fg.setShared(fg_block, HICANN.V_gmax1, 50)
    fg.setShared(fg_block, HICANN.V_gmax2, 50)
    fg.setShared(fg_block, HICANN.V_gmax3, 50)
    fg.setShared(fg_block, HICANN.V_reset, 100) # Reset voltage

    fg.setNeuron(neuron_c, HICANN.V_syntcx, 800)
    fg.setNeuron(neuron_c, HICANN.V_syntci, 800)
    fg.setNeuron(neuron_c, HICANN.V_t, 1000) # Threshold voltage
    fg.setNeuron(neuron_c, HICANN.E_l, 100) # Leakage voltage
    fg.setNeuron(neuron_c, HICANN.I_gl, 10) # Time constant

else:
    for fg_block in Coordinate.iter_all(Coordinate.FGBlockOnHICANN):
        hicann.floating_gates.setShared(fg_block, pyhalbe.HICANN.V_dllres, 300)
        hicann.floating_gates.setShared(fg_block, pyhalbe.HICANN.I_breset,      1023)
        hicann.floating_gates.setShared(fg_block, pyhalbe.HICANN.I_bstim,       1023)
        hicann.floating_gates.setShared(fg_block, pyhalbe.HICANN.int_op_bias,   1023)
        try:
            hicann.floating_gates.setShared(fg_block, pyhalbe.HICANN.V_bout,        1023)
        except:
            pass
        try:
            hicann.floating_gates.setShared(fg_block, pyhalbe.HICANN.V_bexp,        900)
        except:
            pass
        hicann.floating_gates.setShared(fg_block, pyhalbe.HICANN.V_br,             0)
        hicann.floating_gates.setShared(fg_block, pyhalbe.HICANN.V_bstdf,          0)
        hicann.floating_gates.setShared(fg_block, pyhalbe.HICANN.V_ccas,         800)
        try:
            hicann.floating_gates.setShared(fg_block, pyhalbe.HICANN.V_clrc,           0)
        except:
            pass
        try:
            hicann.floating_gates.setShared(fg_block, pyhalbe.HICANN.V_clra,           0)
        except:
            pass
        hicann.floating_gates.setShared(fg_block, pyhalbe.HICANN.V_dep,            0)
        hicann.floating_gates.setShared(fg_block, pyhalbe.HICANN.V_dllres,       200)
        hicann.floating_gates.setShared(fg_block, pyhalbe.HICANN.V_dtc,            0)
        hicann.floating_gates.setShared(fg_block, pyhalbe.HICANN.V_fac,            0)
        hicann.floating_gates.setShared(fg_block, pyhalbe.HICANN.V_gmax0,       50)
        hicann.floating_gates.setShared(fg_block, pyhalbe.HICANN.V_gmax1,       50)
        hicann.floating_gates.setShared(fg_block, pyhalbe.HICANN.V_gmax2,       50)
        hicann.floating_gates.setShared(fg_block, pyhalbe.HICANN.V_gmax3,       50)
        hicann.floating_gates.setShared(fg_block, pyhalbe.HICANN.V_m,              0)
        hicann.floating_gates.setShared(fg_block, pyhalbe.HICANN.V_reset,        226)
        hicann.floating_gates.setShared(fg_block, pyhalbe.HICANN.V_stdf,           0)
        hicann.floating_gates.setShared(fg_block, pyhalbe.HICANN.V_thigh,          0)
        hicann.floating_gates.setShared(fg_block, pyhalbe.HICANN.V_tlow,           0)

    # for Coord.NeuronOnHICANN(Coord.X(0), Coord.top)

    hicann.floating_gates.setNeuron(neuron_c, pyhalbe.HICANN.E_l     ,395           )
    hicann.floating_gates.setNeuron(neuron_c, pyhalbe.HICANN.E_syni  ,342           )
    hicann.floating_gates.setNeuron(neuron_c, pyhalbe.HICANN.E_synx  ,490           )
    hicann.floating_gates.setNeuron(neuron_c, pyhalbe.HICANN.I_bexp  ,0             )
    hicann.floating_gates.setNeuron(neuron_c, pyhalbe.HICANN.I_convi ,1023          )
    hicann.floating_gates.setNeuron(neuron_c, pyhalbe.HICANN.I_convx ,1023          )
    hicann.floating_gates.setNeuron(neuron_c, pyhalbe.HICANN.I_fire  ,0             )
    hicann.floating_gates.setNeuron(neuron_c, pyhalbe.HICANN.I_gl    ,50            )
    hicann.floating_gates.setNeuron(neuron_c, pyhalbe.HICANN.I_gladapt,       0     )
    hicann.floating_gates.setNeuron(neuron_c, pyhalbe.HICANN.I_intbbi ,       511   )
    hicann.floating_gates.setNeuron(neuron_c, pyhalbe.HICANN.I_intbbx ,       511   )
    hicann.floating_gates.setNeuron(neuron_c, pyhalbe.HICANN.I_pl    ,100           )
    hicann.floating_gates.setNeuron(neuron_c, pyhalbe.HICANN.I_radapt,        1023  )
    hicann.floating_gates.setNeuron(neuron_c, pyhalbe.HICANN.I_rexp  ,0             )
    hicann.floating_gates.setNeuron(neuron_c, pyhalbe.HICANN.I_spikeamp,      1023  )
    hicann.floating_gates.setNeuron(neuron_c, pyhalbe.HICANN.V_exp   ,0             )
    hicann.floating_gates.setNeuron(neuron_c, pyhalbe.HICANN.V_syni  ,511           )
    hicann.floating_gates.setNeuron(neuron_c, pyhalbe.HICANN.V_syntci,        820   )
    hicann.floating_gates.setNeuron(neuron_c, pyhalbe.HICANN.V_syntcx,        820   )
    hicann.floating_gates.setNeuron(neuron_c, pyhalbe.HICANN.V_synx  ,511           )
    hicann.floating_gates.setNeuron(neuron_c, pyhalbe.HICANN.V_t     ,819           )


# Configure two ADCs to read voltages of our neurons
hicann.enable_aout(neuron_c, Coordinate.AnalogOnHICANN(0))

# Setup Gbit Links and merger tree
sending_link = Coordinate.GbitLinkOnHICANN(0)
hicann.layer1[sending_link] = pyhalbe.HICANN.GbitLink.Direction.TO_HICANN

receiving_link = Coordinate.GbitLinkOnHICANN(1)
hicann.layer1[receiving_link] = pyhalbe.HICANN.GbitLink.Direction.TO_DNC

for m in Coordinate.iter_all(Coordinate.DNCMergerOnHICANN):
     m = hicann.layer1[merger]
     m.config = m.MERGE
     m.slow = False
     m.loopback = False

# sending link should loopback to receiving link
m = hicann.layer1[Coordinate.DNCMergerOnHICANN(sending_link.value())]
m.loopback = True

# Construct input spike train
spikes_raw = np.arange(20) * 1e-6 + 10e-6
more_spikes_raw = np.arange(20) * 1e-6 + spikes_raw[-1]*2
spikes_raw = np.concatenate([spikes_raw, more_spikes_raw])
duration  = spikes_raw[-1] + 30e-6

spikes = pysthal.Vector_Spike()
for t in spikes_raw:
    spikes.append(pysthal.Spike(in_addr, t))

hicann.sendSpikes(sending_link, spikes)

print hicann

# Connect to hardware
connection_db = pysthal.MagicHardwareDatabase()
wafer.connect(connection_db)

class NoneConfigurator(pysthal.HICANNConfigurator):
    def config(self, fpga, h, hicann):

        self.hicann_init(h)

        self.config_synapse_array(h, hicann);

        self.config_neuron_quads(h, hicann, True)
        self.config_phase(h, hicann)
        self.config_gbitlink(h, hicann)
        self.config_synapse_drivers(h, hicann)
        self.config_synapse_switch(h, hicann)
        self.config_crossbar_switches(h, hicann)
        self.config_repeater(h, hicann)
        self.config_merger_tree(h, hicann)
        self.config_dncmerger(h, hicann)
        self.config_background_generators(h, hicann)
        self.flush_hicann(h)
        self.lock_repeater(h, hicann)
        self.config_neuron_config(h, hicann)
        self.config_neuron_quads(h, hicann)
        self.config_analog_readout(h, hicann)
        self.flush_hicann(h)


#class FooConfigurator(pysthal.HICANNConfigurator):
#    def config(self, fpga, handle, data):
#        pyhalbe.HICANN.init(handle, False)
#	self.config_synapse_drivers(handle, data);

wafer.configure(pysthal.HICANNConfigurator())
#wafer.configure(NoneConfigurator())
#wafer.configure(FooConfigurator())

# Run experiment:
runner = pysthal.ExperimentRunner(duration)

# Setup ADC
recorder0 = hicann.analogRecorder(Coordinate.AnalogOnHICANN(0))
recorder0.activateTrigger(duration)

wafer.start(runner)

outdir_str = "in_addr_"+str(in_addr_int) + "_" + args.vertical

try:
    os.mkdir(outdir_str)
except:
    pass

# Write voltage trace
np.savetxt(os.path.join(outdir_str,"voltages.csv"), zip(recorder0.getTimestamps(),recorder0.trace()))

try:
    # … and spikes
    spikes = hicann.receivedSpikes(receiving_link)
    times = [a.time for a in spikes]
    addrs = [a.addr.value() for a in spikes]
    np.savetxt(os.path.join(outdir_str,"spikes.csv"), zip(times, addrs))
except Exception as e:
    print e

wafer.dump("wafer.xml",True)
