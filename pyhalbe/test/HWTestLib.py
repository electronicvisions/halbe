import time as t
import numpy as np
from pyhalbe import *
import pyhalco_hicann_v2 as Coordinate
from pyhalco_common import Enum
from HWTest import HWTest
from matplotlib import pylab
from scipy import ndimage

"""@@@@@@@@@@ SIMPLE OFFLINE FUNCTIONS @@@@@@@@@@"""
# returns an FGControl with parameters for reset potential and refractory period detection
def LoadResetFGParameters():
    # define floating gate parameters globally
    fgc = HICANN.FGControl()

    for ii in range(512):
        neuron_number = Enum(ii)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.E_l,        400)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.E_syni,     200)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.E_synx,     600)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.I_bexp,     0)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.I_convi,    800)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.I_convx,    800)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.I_fire,     0)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.I_gl,       800)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.I_gladapt,  0)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.I_intbbi,   800)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.I_intbbx,   800)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.I_pl,       0)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.I_radapt,   0)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.I_rexp,     0)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.I_spikeamp, 1023)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.V_exp,      0)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.V_syni,     600)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.V_syntci,   600)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.V_syntcx,   600)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.V_synx,     600)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.V_t,        600)

    for fgbl in [ Enum(zz) for zz in range(4) ]:
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.I_breset,    1023)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.I_bstim,     500)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.int_op_bias, 1023)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_br,        0)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_bstdf,     0)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_ccas,      800)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_dep,       0)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_dllres,    200)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_dtc,       0)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_fac,       0)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_gmax0,     50)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_gmax1,     50)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_gmax2,     50)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_gmax3,     50)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_m,         0)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_reset,     500)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_stdf,      0)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_thigh,     0)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_tlow,      0)

    fgc.setShared(Coordinate.FGBlockOnHICANN(Enum(0)), HICANN.shared_parameter.V_bout, 1023)
    fgc.setShared(Coordinate.FGBlockOnHICANN(Enum(2)), HICANN.shared_parameter.V_bout, 1023)

    return fgc


# returns an FGControl with default floating gate parameters
def LoadDefaultFGParameters():
    # define floating gate parameters globally
    fgc = HICANN.FGControl()
    # "default" FG parameters for all neurons first
    for ii in range(512):
        neuron_number = Enum(ii)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.E_l,        400)#! lowest potential, under reset, so there are no spikes under background
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.E_syni,     200)#! big PSPs linear
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.E_synx,     600)#! big PSPs linear
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.I_bexp,     0)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.I_convi,    800)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.I_convx,    800)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.I_fire,     0)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.I_gl,       800)#! tau_syn ca. 3-4 ms
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.I_gladapt,  0)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.I_intbbi,   800)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.I_intbbx,   800)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.I_pl,       10) #! 30 ms == tau_syn
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.I_radapt,   0)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.I_rexp,     0)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.I_spikeamp, 1023)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.V_exp,      0)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.V_syni,     600)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.V_syntci,   700)# 30 ms
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.V_syntcx,   700)# 30 ms
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.V_synx,     600)
        fgc.setNeuron(Coordinate.NeuronOnHICANN(neuron_number), HICANN.neuron_parameter.V_t,        1000)#! close to reset, but above it

    for fgbl in [ Enum(zz) for zz in range(4) ]:
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.I_breset,    1023)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.I_bstim,     500)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.int_op_bias, 1023)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_br,        0)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_bstdf,     0)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_ccas,      800)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_dep,       0)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_dllres,    200)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_dtc,       0)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_fac,       0)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_gmax0,     50)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_gmax1,     50)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_gmax2,     50)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_gmax3,     50)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_m,         0)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_reset,     500)#! over rest potential
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_stdf,      0)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_thigh,     0)
        fgc.setShared(Coordinate.FGBlockOnHICANN(fgbl), HICANN.shared_parameter.V_tlow,      0)

    fgc.setShared(Coordinate.FGBlockOnHICANN(Enum(0)), HICANN.shared_parameter.V_bout, 1023)
    fgc.setShared(Coordinate.FGBlockOnHICANN(Enum(2)), HICANN.shared_parameter.V_bout, 1023)

    return fgc


# returns synapse WeightRow initialized to 0 and DecoderDoubleRow initialized to 0xf
def LoadDefaultSynapses():
    drow = HICANN.DecoderDoubleRow()
    wrow = [HICANN.WeightRow(), HICANN.WeightRow()]
    for i in range(len(drow[0])):
        drow[0][i] = 15 # initialize with blocking values
        drow[1][i] = 15 # initialize with blocking values
        wrow[0][i] = 0 # initialize in off-mode
        wrow[1][i] = 0 # initialize in off-mode
    return (wrow, drow)


# transforms output from ReadoutFIFO to (l1address, time) where time us in microseconds
def TransformPulses(rec_pulses, with_zeros=False):
    addr = [[]] * len(rec_pulses) #address container for spikes
    time = [[]] * len(rec_pulses) #time container for spikes
    for ii in range(len(rec_pulses)):
        for jj in range(rec_pulses[ii].size()):
            pulse = rec_pulses[ii].get(jj)
            if (int(pulse.getNeuronAddress()) or with_zeros): #save if non-zero pulse
                addr[ii] = addr[ii] + [int(pulse.getNeuronAddress())]
                time[ii] = time[ii] + [pulse.getTime()*0.0066666666] #WARNING! Clock-dependency!!! Here 150MHz
    return (addr, time)


# shows a distribution of a membrane potential
def HistogrammizeTrace(voltage):
    f1 = pylab.figure()
    ax1 = f1.add_subplot(111)
    ax1.set_xlabel("Voltage, [V]")
    ax1.set_ylabel("Probability")
    ax1.set_title("Membrane Distribution")
    hist, bins = np.histogram(voltage, bins = 30)
    width = 0.7*(bins[1]-bins[0])
    center = (bins[:-1]+bins[1:])/2
    ax1.bar(center, hist, align = 'center', width = width)
    pylab.show()


# smoothes the signal
def Smooth(signal, radius = 10, sigma = 3):
    r = range(-int(radius/2), int(radius/2)+1)
    gauss = [1/(sigma*np.sqrt(2*np.pi)) * np.exp(-float(x)**2/(2*sigma**2)) for x in r]
    smsig=np.convolve(signal, gauss, 'same')
    return smsig


# smoothes the signal
def SmoothSignal(signal, time = [], radius = 15, sigma = 2):
    r = range(-int(radius/2), int(radius/2)+1)
    gauss = [1/(sigma*np.sqrt(2*np.pi)) * np.exp(-float(x)**2/(2*sigma**2)) for x in r]
    smsig=np.convolve(signal, gauss)
    return (smsig[radius:len(signal)-radius], time[radius:len(signal)-radius])


# detect edges in the voltage trace, returns positions of edges
def DetectEdges(signal):
    sobel = Smooth(50 * ndimage.sobel(signal, axis=0, mode='nearest'))
    maximum = max(sobel) #maximum and minimum here are vice-versa from those of the signal!!!
    maxpos = [i-1 for i, j in enumerate(sobel) if j == maximum]
    minimum = min(sobel)
    minpos = [i-1 for i, j in enumerate(sobel) if j == minimum]
    return (maxpos[0], minpos[0], sobel)


# detect several edges in the voltage trace to determine refractory period
def DetectMultipleEdges(signal):
    sobel = Smooth(50 * ndimage.sobel(signal, axis=0, mode='nearest'))
    for i in range(len(sobel)):
        if (sobel[i] > -0.2): sobel[i] = 0
    sobel2 = sobel.tolist()

    edges = []; minimum = -1
    while (minimum < 0):
        minimum = min(sobel2)
        if (minimum == 0): break
        minind = sobel2.index(minimum)
        edges.append(minind)
        for j in range(max(minind-11,0), min(minind+12,len(sobel2))):
            sobel2[j] = 0
    edges.sort()

    # debug output
    #~ print edges
    #~ pylab.plot(signal)
    #~ pylab.plot(sobel)
    #~ pylab.show()

    return edges


"""@@@@@@@@@@ SIMPLE HICANN FUNCTIONS @@@@@@@@@@"""
# function that programs the contents of fgctrl to hardware
def ProgramFG(h, fgctrl):
    HICANN.set_fg_values(h, fgctrl)
    HICANN.flush(h) #flush to hardware


# strats the trace FIFO for a period of time (in microseconds), returns FPGA.PulseEventContainer
def ReadoutFIFO(h, channel, readtime, iterations, silent):
    gl = HICANN.GbitLink()
    gl.dirs[channel] = HICANN.GbitLink.Direction.TO_DNC

    gr = DNC.GbitReticle()
    gr[h.to_HICANNOnDNC()] = gl

    HICANN.set_gbit_link(h, gl)
    DNC.set_hicann_directions(h.to_DNCGlobal(), gr)
    HICANN.flush(h) #flush to hardware

    rec_pulses = [FPGA.PulseEventContainer()] * iterations

    for i in range(iterations):
        rec_pulses[i] = FPGA.receive(h.fpga(), h.to_DNCOnFPGA(), readtime)
        if (not silent):
            print "Iteration " + str(i) + ": Received " + str(rec_pulses[i].size()) + " pulses\n"

    gl.dirs[0] = HICANN.GbitLink.Direction.OFF
    gr[0] = gl
    HICANN.set_gbit_link(h, gl)
    DNC.set_hicann_directions(h.to_DNCGlobal(), gr)
    HICANN.flush(h) #flush to hardware

    return rec_pulses


# configures "one-on-one' state in the merger tree, sets phase bits
def SetMergerTree(h):
    tree = HICANN.MergerTree()
    phase = HICANN.phase_t(0)
    HICANN.set_merger_tree(h, tree)
    HICANN.set_phase(h, phase)
    HICANN.flush(h) #flush to hardware


# configures DNC mergers
def SetDNCMergers(h, activate_dnc_input):
    mergers = HICANN.DNCMergerLine()
    for i in range(8):
        mer = Coordinate.DNCMergerOnHICANN(i)
        mergers[mer].slow = True
        mergers[mer].loopback = False
        if (activate_dnc_input):
            mergers[mer].config = HICANN.Merger.MERGE
        else:
            mergers[mer].config = HICANN.Merger.RIGHT_ONLY
    HICANN.set_dnc_merger(h, mergers)


# configures all mergers on HICANN to feed forward, DNC mergers only forward HICANN-internal events
def SetMergerTreeDefaults(h, activate_dnc_input = False):
    SetMergerTree(h)
    SetDNCMergers(h, activate_dnc_input)


# turns all sending repeaters on independent of their input
def SetSendingRepeaterDefaults(h):
    HICANN.set_repeater_block(h, Coordinate.RepeaterBlockOnHICANN(Coordinate.RepeaterBlockOnHICANN(Coordinate.X(0), Coordinate.Y(1))), HICANN.RepeaterBlock())

    sr = HICANN.HorizontalRepeater()
    sr.setOutput(Coordinate.right)
    HICANN.set_repeater(h, Coordinate.HRepeaterOnHICANN(Coordinate.HLineOnHICANN(6),  Coordinate.left), sr)
    HICANN.set_repeater(h, Coordinate.HRepeaterOnHICANN(Coordinate.HLineOnHICANN(14), Coordinate.left), sr)
    HICANN.set_repeater(h, Coordinate.HRepeaterOnHICANN(Coordinate.HLineOnHICANN(22), Coordinate.left), sr)
    HICANN.set_repeater(h, Coordinate.HRepeaterOnHICANN(Coordinate.HLineOnHICANN(30), Coordinate.left), sr)
    HICANN.set_repeater(h, Coordinate.HRepeaterOnHICANN(Coordinate.HLineOnHICANN(38), Coordinate.left), sr)
    HICANN.set_repeater(h, Coordinate.HRepeaterOnHICANN(Coordinate.HLineOnHICANN(46), Coordinate.left), sr)
    HICANN.set_repeater(h, Coordinate.HRepeaterOnHICANN(Coordinate.HLineOnHICANN(54), Coordinate.left), sr)
    HICANN.set_repeater(h, Coordinate.HRepeaterOnHICANN(Coordinate.HLineOnHICANN(62), Coordinate.left), sr)
    HICANN.flush(h) #flush to hardware


# sets a switch in the left synapse switch
def SetLeftSyndriverSwitchLine(h, vwire, syndrvnumber):
    sw = HICANN.SynapseSwitch()
    addr = (Coordinate.SynapseSwitchRowOnHICANN(Coordinate.Y(syndrvnumber), Coordinate.left))
    sw.set(Coordinate.VLineOnHICANN(vwire), addr.line(), True)
    srow_cfg = sw.get_row(addr)
    HICANN.set_syndriver_switch_row(h, addr, srow_cfg)
    HICANN.flush(h) #flush to hardware


# sets a switch in the left crossbar
def SetLeftCrossbarLine(h, vwire, hwire):
    cb = HICANN.Crossbar()
    cb.set(Coordinate.VLineOnHICANN(vwire), Coordinate.HLineOnHICANN(hwire), True)
    row_cfg = cb.get_row(Coordinate.HLineOnHICANN(hwire), Coordinate.left)
    HICANN.set_crossbar_switch_row(h, Coordinate.HLineOnHICANN(hwire), Coordinate.left, row_cfg)
    HICANN.flush(h) #flush to hardware


# sets a switch in the right synapse switch
def SetRightSyndriverSwitchLine(h, vwire, syndrvnumber):
    sw = HICANN.SynapseSwitch()
    addr = (Coordinate.SynapseSwitchRowOnHICANN(Coordinate.Y(syndrvnumber), Coordinate.right))
    sw.set(Coordinate.VLineOnHICANN(vwire), addr.line(), True)
    srow_cfg = sw.get_row(addr)
    HICANN.set_syndriver_switch_row(h, addr, srow_cfg)
    HICANN.flush(h) #flush to hardware


# sets a switch in the right crossbar
def SetRightCrossbarLine(h, vwire, hwire):
    cb = HICANN.Crossbar()
    cb.set(Coordinate.VLineOnHICANN(vwire), Coordinate.HLineOnHICANN(hwire), True)
    row_cfg = cb.get_row(Coordinate.HLineOnHICANN(hwire), Coordinate.right)
    HICANN.set_crossbar_switch_row(h, Coordinate.HLineOnHICANN(hwire), Coordinate.right, row_cfg)
    HICANN.flush(h) #flush to hardware


# sets a double line of synapses (weights and decoders)
def SetLeftSynapseDoubleLine(h, syndrvnumber, weightdrow, decoderdrow):

    # Save according coordinates in variables
    syndrv_coord = Coordinate.SynapseDriverOnHICANN(Coordinate.Y(syndrvnumber), Coordinate.left)
    synrow_top = Coordinate.SynapseRowOnHICANN(syndrv_coord, Coordinate.top)
    synrow_bottom = Coordinate.SynapseRowOnHICANN(syndrv_coord, Coordinate.bottom)

    HICANN.set_decoder_double_row(h,
                                  HICANN.SynapseController(),
                                  syndrv_coord,
                                  decoderdrow)
    HICANN.set_weights_row(h,
                           HICANN.SynapseController(),
                           synrow_top,
                           weightdrow[0])
    HICANN.set_weights_row(h,
                           HICANN.SynapseController(),
                           synrow_bottom,
                           weightdrow[1])
    HICANN.flush(h) #flush to hardware


# configures a synapse driver for certain purposes. decoder and FG-gmax-source is always 0
def ConfigLeftDriver(h, drvnumber, mirror, listen, gmaxdiv, topex, topin, botex, botin):
    driver = HICANN.SynapseDriver()
    driver.set_l1()
    if (listen): driver.set_listen()
    if (mirror): driver.set_l1_mirror()

    for yy in [ Coordinate.top, Coordinate.bottom ]:
        for xx in [ Coordinate.left, Coordinate.right ]:
            driver[yy].set_syn_in(xx, topex)
            driver[yy].set_gmax_div(xx, gmaxdiv)
        for xx in [ Coordinate.top, Coordinate.bottom ]:
            driver[yy].set_decoder(xx, HICANN.DriverDecoder(0))
            driver[yy].set_gmax(0)

    # set_synapse_driver includes toggling the drivers DLL
    HICANN.set_synapse_driver(h, Coordinate.SynapseDriverOnHICANN(Coordinate.Y(drvnumber), Coordinate.left), driver)
    HICANN.flush(h) #flush to hardware


# configures an analog output
def ConfigureAoutForNeuron(h, aout_number, neuron):
    aout = HICANN.Analog()
    aout.enable(Coordinate.AnalogOnHICANN(aout_number))
    if (neuron > 255):
        if (neuron%2): aout.set_membrane_bot_odd(Coordinate.AnalogOnHICANN(aout_number))
        else: aout.set_membrane_bot_even(Coordinate.AnalogOnHICANN(aout_number))
    else:
        if (neuron%2): aout.set_membrane_top_odd(Coordinate.AnalogOnHICANN(aout_number))
        else: aout.set_membrane_top_even(Coordinate.AnalogOnHICANN(aout_number))
    HICANN.set_analog(h, aout)
    HICANN.flush(h) #flush to hardware


# configures a neuron: only one (!) neuron per NeurnQuad
def ConfigureNeuron(h, neuron, l1address, current_input, aout_enable):
    nconf = HICANN.NeuronConfig()
    nconf.bigcap[Coordinate.top.value()]       = True # use small capacitance
    nconf.bigcap[Coordinate.bottom.value()]    = True
    nconf.fast_I_gl[Coordinate.top.value()]    = True #use fastest membrane possible
    nconf.fast_I_gl[Coordinate.bottom.value()] = True
    nconf.slow_I_gl[Coordinate.top.value()]    = False
    nconf.slow_I_gl[Coordinate.bottom.value()] = False
    HICANN.set_neuron_config(h, nconf)

    nquad = HICANN.NeuronQuad()
    nactive = HICANN.Neuron()

    nactive.address(HICANN.L1Address(l1address))
    nactive.activate_firing(True)
    nactive.enable_fire_input(False)
    nactive.enable_current_input(current_input)
    nactive.enable_aout(aout_enable)

    if (neuron > 255):
        nquad[Coordinate.NeuronOnQuad(Coordinate.X(neuron%2), Coordinate.Y(1))] = nactive # only one neuron fires
        HICANN.set_denmem_quad(h, Coordinate.QuadOnHICANN((neuron-256)/2), nquad)
    else:
        nquad[Coordinate.NeuronOnQuad(Coordinate.X(neuron%2), Coordinate.Y(0))] = nactive # only one neuron fires
        HICANN.set_denmem_quad(h, Coordinate.QuadOnHICANN(neuron/2), nquad)
    HICANN.flush(h) #flush to hardware


# stimulates a neuron with current, does NOT configure the neuron!
def StimulateNeuronWithCurrent(h, neuron, strength):
    stimulus = HICANN.FGStimulus()
    block = None
    if (neuron > 255):
        if (neuron%2): block  = Coordinate.FGBlockOnHICANN(Enum(3))
        else: block = Coordinate.FGBlockOnHICANN(Enum(2))
    else:
        if (neuron%2): block = Coordinate.FGBlockOnHICANN(Enum(1))
        else: block = Coordinate.FGBlockOnHICANN(Enum(0))

    stimulus.setPulselength(15)
    stimulus.setContinuous(True)
    stimulus[:] = [strength] * 129
    HICANN.set_current_stimulus(h, block, stimulus)
    HICANN.flush(h) #flush to hardware


# stimulates a neuron with step current, strength is 0-1023, length is 0-129 does NOT configure the neuron!
def StimulateNeuronWithStepCurrent(h, neuron, strength, length):
    stimulus = HICANN.FGStimulus()
    block = None
    if (neuron > 255):
        if (neuron%2): block = Coordinate.FGBlockOnHICANN(Enum(3))
        else: block = Coordinate.FGBlockOnHICANN(Enum(2))
    else:
        if (neuron%2): block = Coordinate.FGBlockOnHICANN(Enum(1))
        else: block = Coordinate.FGBlockOnHICANN(Enum(0))

    stimulus.setPulselength(15)
    stimulus.setContinuous(True)
    stimulus[:length] = [strength] * length
    stimulus[length:] = [0] * (len(stimulus) - length)
    HICANN.set_current_stimulus(h, block, stimulus)
    HICANN.flush(h) #flush to hardware


# starts all BEGs with same configuration
def StartAllBEG(h, period, addr, random):
    bgarray = HICANN.BackgroundGeneratorArray()
    for i in range(8):
        bgarray[i].enable(True)
        bgarray[i].random(random)
        bgarray[i].seed(200)
        bgarray[i].period(period)
        bgarray[i].address(HICANN.L1Address(addr))
    HICANN.set_background_generator(h, bgarray)
    HICANN.flush(h) #flush to hardware


# configures HICANN<->DNC link and starts FPGA BEG
def StartFPGABEG(h, d, f, period, first_addr, last_addr, random, channel):
    #configure all DNC<->HICANN links (excessive, but ok for now)
    gbit = DNC.GbitReticle()
    link = HICANN.GbitLink()
    for i in range(8): link.dirs[i] = HICANN.GbitLink.Direction.TO_HICANN #all links
    for i in range(8):
        gbit[Coordinate.HICANNOnDNC(Enum(i))] = link
    HICANN.set_gbit_link(h, link)
    DNC.set_hicann_directions(h.to_DNCGlobal(), gbit)

    #configure FPGA Background Generator
    bg = FPGA.BackgroundGenerator()
    bg.enable        = True
    bg.poisson       = random
    bg.seed          = 12345 #cannot be zero
    bg.rate          = period   #in DNC-clock cycles
    bg.first_address = first_addr
    bg.last_address  = last_addr #last address is greater or equal than first
    bg.hicann_number = h.coordinate()
    bg.channels[channel] = True #set active channel
    FPGA.set_fpga_background_generator(f, bg)
    HICANN.flush(h) #flush to hardware


# stops FPGA BEG and HICANN<->DNC link to prevent pulse/config packet mixup
def StopFPGABEG(h, d, f):
    gbit = DNC.GbitReticle()
    link = HICANN.GbitLink()
    bg = FPGA.BackgroundGenerator()

    bg.enable = False
    FPGA.set_fpga_background_generator(f, bg)

    for i in range(8): link.dirs[i] = HICANN.GbitLink.Direction.OFF
    for i in range(8):
        gbit[Coordinate.HICANNOnDNC(Enum(i))] = link
    HICANN.set_gbit_link(h, link)
    DNC.set_hicann_directions(h.to_DNCGlobal(), gbit)
    HICANN.flush(h) #flush to hardware


# returns a trace from an ADC-channel, also applies quick transformation DAC-values -> voltage
# and transforms time to microseconds
def GetADCTrace(length, channel):
    adc  = Handle.ADC()
    chan = [Coordinate.ChannelOnADC(5), Coordinate.ChannelOnADC(7)]
    trig = Coordinate.TriggerOnADC(0)
    conf = ADC.Config(int(length * 500 / 10.3 + 1) * 2, chan[channel], trig)
    ADC.config(adc, conf)
    ADC.trigger_now(adc)
    trace = ADC.get_trace(adc)
    # ADC quick linear calibration earmarks Endvalue=(a*value+b)
    a = -0.000694 # standard values
    b = 2.1
    v = a * trace + b
    t = [0.] * len(trace)
    for i in range(len(trace)):
        t[i] = i * 0.01733853 #time in microseconds
    return (v, t)


"""@@@@@@@@@@ COMPLEX HICANN FUNCTIONS @@@@@@@@@@"""
# takes the activation curve of a neuron, saves into file
def RecordActivationCurve(h, stimbegin, stimend, period, neuron, program_fg, fgc, filename):
    FPGA.reset(h.fpga())
    HICANN.reset(h)
    HICANN.init(h, False)

    # Program floating gates if necessary
    if (program_fg):
        ProgramFG(h, fgc)

    # Set all mergers to forward directly downwards
    SetMergerTreeDefaults(h)

    # Configure Background Generators
    bgarray = HICANN.BackgroundGeneratorArray()
    for i in range(6,8):
        bgarray[i].enable(True)
        bgarray[i].random(True)
        bgarray[i].seed(200*i)
        bgarray[i].period(period-6+i) # desynchronise both BEGs
        bgarray[i].address(HICANN.L1Address(0))
        HICANN.set_background_generator(h, bgarray) # BG7 (exitatory), BG6 (inhibitory)

    # Enable Repeaters
    SetSendingRepeaterDefaults(h)

    # Syndriver Switch
    line0 = 111
    line1 = 109
    if (neuron > 255):
        line0 = 112
        line1 = 114
    SetLeftSyndriverSwitchLine(h, 28, line0)
    SetLeftSyndriverSwitchLine(h, 24, line1)

    # Synapse Driver
    ConfigLeftDriver(h, line0, False, False, 1, True, False, True, False) # BG7 -> synapse0
    ConfigLeftDriver(h, line1, False, False, 1, False, True, False, False) # BG6 -> synapse1

    # Synapses
    (wrow, drow) = LoadDefaultSynapses()

    nn = neuron
    if (neuron > 255): nn = neuron - 256

    drow[0][nn] = 0
    wrow[0][nn] = 15

    SetLeftSynapseDoubleLine(h, line0, wrow, drow)
    SetLeftSynapseDoubleLine(h, line1, wrow, drow)

    # Neurons
    ConfigureNeuron(h, neuron, 32, True, True)

    # Sweep stimulus and readout spikes
    iterations = 100
    readtimes = 100
    spikenumbers = np.zeros(stimend-stimbegin, dtype=int)

    for stim in range(stimbegin, stimend):
        StimulateNeuronWithCurrent(h, neuron, stim)
        HICANN.flush(h) #flush to hardware
        t.sleep(0.0001) # wait for neurons to start spiking
        spikes = ReadoutFIFO(h, 0, readtimes, iterations, True)
        for i in range(iterations):
            spikenumbers[stim-stimbegin] += spikes[i].size()
        print "Stimulus " + str(stim) + ": " + str(spikenumbers[stim-stimbegin])

    # Plot the curve and save it
    fig = pylab.figure()
    ax = fig.add_subplot(1, 1, 1)
    ax.set_position([0.15, 0.15, 0.7, 0.7])

    ax.plot(range(stimbegin, stimend), spikenumbers, label='Neuron ' + str(neuron), linewidth=2)

    ax.set_xlabel('Mean Membrane Potential, [mV]', size=16)
    ax.set_ylabel('Spike Frequency, [kHz]', size=16)
    ax.set_title('Neurosampling Activation Curve', size=20)
    ax.title.set_y(1.05)
    ax.legend(loc='upper left')
    #ax.set_xlim(0,1023)

    fig.savefig("plot_activation_curve_" + str(filename) + ".pdf", dpi=(1024/8))
    pylab.show()


# puts a neuron to an analog output, also activates neuron in the first place
def PutNeuronToAout(h, neuron, aout_number, current_input):
    ConfigureNeuron(h, neuron, 32, current_input, True)
    ConfigureAoutForNeuron(h, aout_number, neuron)


# takes the linearity curve of synaptic input
def TakeLinearityCurve(h, neuron, true_if_inh, gmaxdiv, aver_length):
    ints = [[]] * 16
    for w in range(16):
        (voltage, time) = GetAveragePSP(h, neuron, w, true_if_inh, aver_length, True, gmaxdiv)
        # take the lowest/highest 50% of the trace and get the median, assume this is the offset of the PSP
        temp = list(voltage)
        temp.sort()
        offset = 0
        if (true_if_inh): offset = np.median(temp[int(len(temp)*5/10):])
        else: offset = np.median(temp[:int(len(temp)*5/10)])
        ints[w] = np.trapz(voltage) - np.trapz([offset] * len(voltage))

        #debug-output
        #~ print "Offset is " + str(offset) + " Volts"
        #~ print "Integral is " + str(ints[w]) + " V*us"
        #~ pylab.plot(time, voltage)
        #~ pylab.plot(time, [offset] * len(time))
        #~ pylab.show()

    return ints


# averages over several PSPs and returns the average
def GetAveragePSP(h, neuron, weight, true_if_inhib, aver_length, smooth, gmaxdiv=1):
    #HICANN.reset(h)
    #HICANN.init(h, False)
    number_of_psps = int(aver_length/30)
    number_of_actual_psps = 0
    points_per_psp = 1300
    points_backwards = int(points_per_psp/4) #1/4 backwards and 3/4 forward
    points_forward = int(points_per_psp*3/4)

    if true_if_inhib: StimulateNeuronWithSynapses(h, neuron, False, 4000, False, True, weight, gmaxdiv)
    else: StimulateNeuronWithSynapses(h, neuron, False, 4000, True, False, weight, gmaxdiv)

    PutNeuronToAout(h, neuron, 1, False)
    (voltage, time) = GetADCTrace(aver_length, 1)
    if (smooth):
        (voltage, time) = SmoothSignal(voltage, time)
    voltage = voltage.tolist()

    psps = [[]] * number_of_psps
    avgpsp = [0] * points_per_psp
    if true_if_inhib:
        for i in range(number_of_psps):
            minind = voltage.index(min(voltage)) #find minimum of i-th PSP
            if ((len(voltage) - minind) > points_forward and minind > points_backwards):
                psps[i] = voltage[minind-points_backwards:minind+points_forward] #copy the psp
                number_of_actual_psps += 1
            else:
                psps[i] = [max(voltage)] * points_per_psp
            voltage[max(0,minind-points_backwards):min(len(voltage)-1,minind+points_forward)] = [max(voltage)]*points_per_psp #delete the psp, evtl. changes the list length...
            for j in range(points_per_psp): #add new psp to average-list
                avgpsp[j] += psps[i][j]
            #pylab.plot(psps[i])
            #pylab.plot(voltage)
            #pylab.show()
    else:
        for i in range(number_of_psps):
            maxind = voltage.index(max(voltage)) #find maximum of i-th PSP
            if ((len(voltage) - maxind) > points_forward and maxind > points_backwards):
                psps[i] = voltage[maxind-points_backwards:maxind+points_forward] #copy the psp
                number_of_actual_psps += 1
            else:
                psps[i] = [min(voltage)] * points_per_psp
            voltage[max(0,maxind-points_backwards):min(len(voltage)-1,maxind+points_forward)] = [min(voltage)]*points_per_psp #delete the psp, evtl. changes the list length...
            for j in range(points_per_psp): #add new psp to average-list
                avgpsp[j] += psps[i][j]
            #pylab.plot(psps[i])
            #pylab.plot(voltage)
            #pylab.show()
    for j in range(points_per_psp):
        avgpsp[j] /= number_of_actual_psps
    # debug output
    #print "Number of PSPs is " + str(number_of_psps)
    #pylab.plot(avgpsp)
    #pylab.plot(voltage)
    #pylab.show()
    return (avgpsp, time[0:points_per_psp])


# calculates the time constant of a neuron, requires non-spiking neurons!!!
def GetMembraneTimeConstant(h, neuron):
    FPGA.reset(h.fpga())
    HICANN.reset(h)
    HICANN.init(h, False)
    channel = 1
    siglength = 1000
    minaveraging = 200
    StimulateNeuronWithStepCurrent(h, neuron, 100, 30)
    PutNeuronToAout(h, neuron, channel, True) # implicit neuron activation here!

    maxind = 0; minind = 0
    voltage = [0]; time = 0; sobel = 0
    while((maxind+siglength >= len(voltage)) or (minind < minaveraging)): # get new signal until it can be extracted
        (voltage, time) = GetADCTrace(100, channel)
        (voltage, time) = SmoothSignal(voltage, time)
        (minind, maxind, sobel) = DetectEdges(voltage)

    minimum = np.mean(voltage[minind-minaveraging:minind-20])

    index = maxind
    trace = voltage[index:index+siglength]
    for i in range(len(trace)):
        trace[i] -= minimum
    time = time[0:siglength]

    from fit import fit_exp
    x = fit_exp(time, trace)

    # debug-output
    #print "With a*exp(-t/tau)+b: [a, tau, b] = " + str(x),
    #fit = [[0]] * len(time)
    #for i in range(len(time)):
    #   fit[i] = x[0] * np.exp(-time[i]/x[1]) + x[2]
    #fit = pylab.array(fit)
    #trace = pylab.array(trace)
    #
    #print "   fit quality:", pylab.mean(fit - trace), pylab.std(fit - trace)
    #
    #pylab.plot(time, trace)
    #pylab.plot(time, fit)
    #pylab.show()

    return x[1]


# calculates the synaptic time constant of a neuron, requires non-spiking neurons!!!
def GetSynapticTimeConstant(h, neuron, true_if_inh):
    from fit import fit_psp, ideal_psp, ideal_psp_fixmem, alpha_psp
    from scipy import optimize
    channel = 1
    taumem = GetMembraneTimeConstant(h, neuron) #get membrane time constant first
    (voltage, time) = GetAveragePSP(h, neuron, 15, true_if_inh, 400, True, 1)

    # take the lowest 50% of the trace and get the median, assume this is the offset of the PSP
    #~ temp = list(voltage)
    #~ temp.sort()
    if (true_if_inh): offset = np.median(voltage[int(len(voltage)*9/10):])
    else: offset = np.median(voltage[:int(len(voltage)*1/10)])

    maxind = voltage.index(max(voltage))
    i = maxind
    value = voltage[i]
    while (value > offset):
        i -= 1
        value = voltage[i]
    t_begin = time[i] #4.1
    t_max = time[maxind] - t_begin

    #old fitting routine
    fitpar = fit_psp(time, voltage, taumem)
    print fitpar
    pylab.plot(time, ideal_psp_fixmem(fitpar, taumem, pylab.array(time)))

    def tmax_func(taus):
        if taus != taumem:
            return (pylab.log(taus) - pylab.log(taumem)) / (1. / taumem - 1. / taus) - t_max
        else:
            return taus - t_max

    result = optimize.leastsq(
        tmax_func, taumem, full_output=1)
    #~ print result
    tausyn = result[0]

    #debug-output
    #~ psp = [[]] * len(time)
    #~ for i in range(len(time)):
        #~ psp[i] = max(voltage) - mean_estimate)/(1./4.) * (np.exp(-time[i] / tausyn) - np.exp(-time[i] / taumem)) + min(voltage)
    #~ print "T_max = " + str(t_max) + " microseconds"
    #~ print "Tau_mem = " + str(taumem) + " microseconds"
    print "Tau_syn = " + str(tausyn) + " microseconds"
    #~ print "PSP begins at " + str(t_begin) + " microseconds"
    pylab.plot(time, alpha_psp((max(voltage)-offset)*0.9, tausyn, taumem, t_begin, offset, pylab.array(time)))
    pylab.plot(time, [offset] * len(time))
    pylab.plot(time, voltage)
    pylab.show()

    return tausyn


# measures and returns the firing threshold of a neuron
def GetFiringThreshold(h, neuron):
    PutNeuronToAout(h, neuron, 1, True)
    StimulateNeuronWithCurrent(h, neuron, 1023)
    (voltage, time) = GetADCTrace(400, 1)

    voltage = voltage.tolist()
    temp = voltage[:] #copy
    temp.sort() #find out where the higher 10% of the trace lies
    maxheight = temp[-1] - temp[0]
    threshold = temp[int(len(voltage)*97/100)]
    hightrace = [0] #copy the highest 10% of the trace out

    for i in range(1,len(voltage)):
        #upper 10% and steep downward edges (resets) stay in the trace the rest is thrown out
        if (voltage[i] > threshold or ((hightrace[-1] == voltage[i-1]) and (voltage[i-1]-voltage[i] > maxheight*0.07))):
            hightrace.append(voltage[i])
    hightrace.pop(0) #delete fake first entry

    #debug output
    #~ print "Trace range is " + str(maxheight) + " Volts"
    #~ print "Threshold is at " + str(threshold) + " Volts"
    #~ pylab.plot(hightrace)
    #~ pylab.plot([threshold] * len(hightrace))
    #~ pylab.show()

    maxima = [] #sequentially delete single peaks and record the maxima
    while(max(hightrace) > threshold):
        maxind = hightrace.index(max(hightrace))
        maxima.append(max(hightrace))
        hightrace[maxind] = threshold
        i = 1
        while(hightrace[maxind-i] > threshold):
            hightrace[maxind-i] = threshold
            i += 1
        i = 1
        while(hightrace[maxind+i] > threshold):
            hightrace[maxind+i] = threshold
            i += 1

        #debug output
        #~ print maxima
        #~ pylab.plot(hightrace)
        #~ pylab.plot([threshold] * len(hightrace))
        #~ pylab.show()

    return np.median(maxima)


# returns the membrane rest potential of a neuron
def GetMembraneRestPotential(h, neuron):
    FPGA.reset(h.fpga())
    HICANN.reset(h)
    HICANN.init(h, False)
    PutNeuronToAout(h, neuron, 1, False)
    (voltage, time) = GetADCTrace(200, 1)
    rest = np.mean(voltage)
    # debug output
    #~ print "Rest potential is at " + str(rest) + "+/-" + str(np.std(voltage)) + " Volts"
    #~ pylab.plot(time, voltage)
    #~ pylab.show()

    return rest


# returns the reset potential of a neuron
def GetResetPotential(h, neuron):
    FPGA.reset(h.fpga())
    HICANN.reset(h)
    HICANN.init(h, False)
    channel = 1
    StimulateNeuronWithCurrent(h, neuron, 1000)
    PutNeuronToAout(h, neuron, channel, True) # implicit neuron activation here!
    (voltage, time) = GetADCTrace(40, channel)
    reset = np.median(voltage)
    # debug output
    #~ print "Reset potential is at " + str(reset) + " Volts"
    #~ pylab.plot(time, voltage)
    #~ pylab.plot(time, [reset] * len(time))
    #~ pylab.show()

    return reset


# returns the length of a refractory period
def GetRefractoryPeriod(h, neuron):
    reset = GetResetPotential(h, neuron)
    (voltage, time) = GetADCTrace(100, 1)
    (voltage, time) = SmoothSignal(voltage, time)
    edges = DetectMultipleEdges(voltage)
    times = [[]] * (len(edges)-1)

    for i in range(len(edges)-1):
        index = edges[i]+10
        value = voltage[index]
        while (value < reset+0.01):
            index += 1
            value = voltage[index]
        times[i] = time[index] - time[edges[i]]
    tauref = np.mean(times)

    # debug output
    #~ print edges
    #~ print times
    #~ print "Reset potential is at " + str(reset) + " Volts"
    #~ print "Refractory period is " + str(tauref) + "+/-" + str(np.std(times)) + " microseconds"
    #~ pylab.plot(time, voltage)
    #~ pylab.show()

    return tauref


# stimulates neuron with excitatory or inhibitory synapses, does NOT configure the neuron!
def StimulateNeuronWithSynapses(h, neuron, random, period, excit, inhib, weight=15, gmaxdiv=1):
    SetMergerTreeDefaults(h)

    # configure Background Generator
    bgarray = HICANN.BackgroundGeneratorArray()
    for i in range(6,8):
        bgarray[i].enable(True)
        bgarray[i].random(random)
        bgarray[i].seed(200)
        bgarray[i].period(period)
        bgarray[i].address(HICANN.L1Address(0))
        HICANN.set_background_generator(h, bgarray) #BG7 (exitatory), BG6 (inhibitory)

    # Enable Repeaters
    SetSendingRepeaterDefaults(h)

    # Crossbar Switch
    SetLeftCrossbarLine(h, 28, 6) #BG7 -> synapse0
    SetLeftCrossbarLine(h, 24, 14) #BG6 -> synapse1

    # Syndriver Switches
    line0 = 111
    line1 = 109
    if (neuron > 255):
        line0 = 112
        line1 = 114
    SetLeftSyndriverSwitchLine(h, 28, line0)
    SetLeftSyndriverSwitchLine(h, 24, line1)

    # Synapse Driver
    ConfigLeftDriver(h, line0, False, False, gmaxdiv, excit, False, excit, False) #BG -> synapse0
    ConfigLeftDriver(h, line1, False, False, gmaxdiv, False, inhib, False, inhib) #BG -> synapse1

    # Synapses
    (wrow, drow) = LoadDefaultSynapses()

    # BG -> neuron0
    nn = neuron
    if (neuron > 255): nn = neuron - 256

    drow[1][nn] = 0
    wrow[1][nn] = weight

    SetLeftSynapseDoubleLine(h, line0, wrow, drow)
    SetLeftSynapseDoubleLine(h, line1, wrow, drow)


# measures frequency of the background generator
def GetBGFrequency(h, period, random, seed):
    FPGA.reset(h.fpga())
    HICANN.reset(h)
    HICANN.init(h, False) #reset everything
    SetMergerTreeDefaults(h)

    # configure Background Generator
    bgarray = HICANN.BackgroundGeneratorArray()
    for i in range(8):
        bgarray[i].enable(True)
        bgarray[i].random(random)
        bgarray[i].seed(seed)
        bgarray[i].period(period)
        bgarray[i].address(HICANN.L1Address(0))
        HICANN.set_background_generator(h, bgarray)
    HICANN.flush(h) #flush to hardware

    iterations = 100
    rec_pulses = ReadoutFIFO(h, 0, 5, iterations, True)
    (addr, time) = TransformPulses(rec_pulses, True)

    freq = [[]] * iterations
    for i in range(iterations):
        trange = time[i][-1] - time[i][0]
        freq[i] = len(time[i])/trange
    std = np.std(freq)
    freq = np.mean(freq)

    #print "Frequency is " + str(freq) + " +/- " + str(std) + " per microsecond"
    return freq


"""@@@@@@@@@@ L1 TESTING FUNCTIONS @@@@@@@@@@"""
# sets Voh and Vol on the vertical setup
def SetL1Voltages(h, vol, voh):
    Support.Power.set_L1_voltages(h, vol, voh)


# uses FPGA BEG to send events into L1 network
def L1Send(h, d, f, period, hline_number, neuron_number):
    if (not(hline_number == 62) and not(hline_number == 54) and not(hline_number == 46) and not(hline_number == 38) and not(hline_number == 30) and not(hline_number == 22) and not(hline_number == 14) and not(hline_number == 6)):
        print "There is no sending repeater on this line!"
    lockperiod = 3000
    channel = 8 - (hline_number + 2)/8

    #configure mergers
    SetMergerTree(h)
    SetDNCMergers(h, True)

    #configure HICANN-background generators
    StartAllBEG(h, lockperiod, 0, False)

    #configure sending repeaters
    SetSendingRepeaterDefaults(h)

    #start FPGA BEG
    StartFPGABEG(h, d, f, period/2, neuron_number, neuron_number, False, channel)
    HICANN.flush(h) #flush to hardware

# uses HICANN BEG to send 0-events into L1 network
def L1SendAll(h, period):
    #configure mergers
    SetMergerTree(h)
    SetDNCMergers(h, False)

    #configure HICANN-background generators
    StartAllBEG(h, period, 0, False)

    #configure sending repeaters
    SetSendingRepeaterDefaults(h)
    HICANN.flush(h) #flush to hardware


# returns 3 events recorded by the repeater
def L1Receive(h, repeater_block, vline_number):
    #switch directions and receiving circuits
    registertype = HICANN.RepeaterBlock.EVEN #which readout-circuit is responsible
    dir = Coordinate.top #transmit-direction
    block = Coordinate.RepeaterBlockOnHICANN(Coordinate.X(0), Coordinate.Y(0))
    if (repeater_block == 4): #bottom left
        registertype = HICANN.RepeaterBlock.EVEN
        dir = Coordinate.bottom
        block = Coordinate.RepeaterBlockOnHICANN(Coordinate.X(0), Coordinate.Y(2))
    elif (repeater_block == 5): #bottom right
        registertype = HICANN.RepeaterBlock.EVEN
        dir = Coordinate.bottom
        block = Coordinate.RepeaterBlockOnHICANN(Coordinate.X(1), Coordinate.Y(2))
    elif (repeater_block == 0): #top left
        registertype = HICANN.RepeaterBlock.ODD
        dir = Coordinate.top
        block = Coordinate.RepeaterBlockOnHICANN(Coordinate.X(0), Coordinate.Y(0))
    elif (repeater_block == 1): #top right
        registertype = HICANN.RepeaterBlock.ODD
        dir = Coordinate.top
        block = Coordinate.RepeaterBlockOnHICANN(Coordinate.X(1), Coordinate.Y(0))

    #configure receiving repeater
    vr = HICANN.VerticalRepeater()
    vr.setInput(dir)
    HICANN.set_repeater(h, Coordinate.VLineOnHICANN(vline_number).repeater(), vr)
    HICANN.flush(h) #flush to hardware

    t.sleep(0.0005) #time for the dll to lock

    #configure receiving repeater block
    rb = HICANN.RepeaterBlock()
    rb.start_tdi[registertype] = False #reset the full flag
    HICANN.set_repeater_block(h, block, rb)
    rb.start_tdi[registertype] = True #start recording received data
    HICANN.set_repeater_block(h, block, rb)
    HICANN.flush(h) #flush to hardware

    t.sleep(0.00001) #recording lasts ca. 10 us

    rb = HICANN.get_repeater_block(h, block)
    test = rb.tdi_data[registertype]

    times = np.zeros(3, dtype=int)
    addresses = np.zeros(3, dtype=int)
    for x in range(3):
        times[x] = test[x].time
        addresses[x] = int(test[x].address)

    #~ print "From repeater " + str(vline_number) + " received: "
    #~ print "Neuron number " + str(test[0].address.value()) + " at time " + str(test[0].time)
    #~ print "Neuron number " + str(test[1].address.value()) + " at time " + str(test[1].time)
    #~ print "Neuron number " + str(test[2].address.value()) + " at time " + str(test[2].time) + "\n"

    rb.start_tdi[registertype] = False #reset the full flag TWO (!) times
    HICANN.set_repeater_block(h, block, rb)
    HICANN.set_repeater_block(h, block, rb)

    #set repeater mode to IDLE to prevent conflicts
    vr.setIdle()
    HICANN.set_repeater(h, Coordinate.VLineOnHICANN(vline_number).repeater(), vr)
    HICANN.flush(h) #flush to hardware

    return times, addresses

# returns 3 events recorded by the repeater
def L1ReceiveVer(h, repeater_block, vline_number):
    #switch directions and receiving circuits
    registertype = HICANN.RepeaterBlock.ODD #which readout-circuit is responsible
    dir = Coordinate.bottom #transmit-direction
    block = Coordinate.RepeaterBlockOnHICANN(Coordinate.X(0), Coordinate.Y(0))
    if (repeater_block == 4): #bottom left
        registertype = HICANN.RepeaterBlock.ODD
        dir = Coordinate.top
        block = Coordinate.RepeaterBlockOnHICANN(Coordinate.X(0), Coordinate.Y(2))
    elif (repeater_block == 5): #bottom right
        registertype = HICANN.RepeaterBlock.ODD
        dir = Coordinate.top
        block = Coordinate.RepeaterBlockOnHICANN(Coordinate.X(1), Coordinate.Y(2))
    elif (repeater_block == 0): #top left
        registertype = HICANN.RepeaterBlock.EVEN
        dir = Coordinate.bottom
        block = Coordinate.RepeaterBlockOnHICANN(Coordinate.X(0), Coordinate.Y(0))
    elif (repeater_block == 1): #top right
        registertype = HICANN.RepeaterBlock.EVEN
        dir = Coordinate.bottom
        block = Coordinate.RepeaterBlockOnHICANN(Coordinate.X(1), Coordinate.Y(0))

    #configure receiving repeater
    vr = HICANN.VerticalRepeater()
    vr.setInput(dir)
    HICANN.set_repeater(h, Coordinate.VLineOnHICANN(vline_number).repeater(), vr)
    HICANN.flush(h) #flush to hardware

    t.sleep(0.0005) #time for the dll to lock

    #configure receiving repeater block
    rb = HICANN.RepeaterBlock()
    rb.start_tdi[registertype] = False #reset the full flag
    HICANN.set_repeater_block(h, block, rb)
    rb.start_tdi[registertype] = True #start recording received data
    HICANN.set_repeater_block(h, block, rb)
    HICANN.flush(h) #flush to hardware

    t.sleep(0.00001) #recording lasts ca. 10 us

    rb = HICANN.get_repeater_block(h, block)
    test = rb.tdi_data[registertype]

    times = np.zeros(3, dtype=int)
    addresses = np.zeros(3, dtype=int)
    for x in range(3):
        times[x] = test[x].time
        addresses[x] = int(test[x].address)

    #~ print "From repeater " + str(vline_number) + " received: "
    #~ print "Neuron number " + str(test[0].address.value()) + " at time " + str(test[0].time)
    #~ print "Neuron number " + str(test[1].address.value()) + " at time " + str(test[1].time)
    #~ print "Neuron number " + str(test[2].address.value()) + " at time " + str(test[2].time) + "\n"

    rb.start_tdi[registertype] = False #reset the full flag TWO (!) times
    HICANN.set_repeater_block(h, block, rb)
    HICANN.set_repeater_block(h, block, rb)

    #set repeater mode to IDLE to prevent conflicts
    vr.setIdle()
    HICANN.set_repeater(h, Coordinate.VLineOnHICANN(vline_number).repeater(), vr)
    HICANN.flush(h) #flush to hardware

    return times, addresses

# returns 3 events recorded by the repeater
def L1ReceiveHor(h, hline_number):
    #switch directions and receiving circuits
    registertype = HICANN.RepeaterBlock.ODD #which readout-circuit is responsible
    dir = Coordinate.right #transmit-direction
    block = Coordinate.RepeaterBlockOnHICANN(Coordinate.X(0), Coordinate.Y(1))

    #configure receiving repeater
    hr = HICANN.HorizontalRepeater()
    hr.setInput(dir)
    HICANN.set_repeater(h, Coordinate.HLineOnHICANN(hline_number).repeater(), hr)
    HICANN.flush(h) #flush to hardware

    t.sleep(0.0005) #time for the dll to lock

    #configure receiving repeater block
    rb = HICANN.RepeaterBlock()
    rb.start_tdi[registertype] = False #reset the full flag
    HICANN.set_repeater_block(h, block, rb)
    rb.start_tdi[registertype] = True #start recording received data
    HICANN.set_repeater_block(h, block, rb)
    HICANN.flush(h) #flush to hardware

    t.sleep(0.00001) #recording lasts ca. 10 us

    rb = HICANN.get_repeater_block(h, block)
    test = rb.tdi_data[registertype]

    times = np.zeros(3, dtype=int)
    addresses = np.zeros(3, dtype=int)
    for x in range(3):
        times[x] = test[x].time
        addresses[x] = int(test[x].address)

    #~ print "From repeater " + str(hline_number) + " received: "
    #~ print "Neuron number " + str(test[0].address.value()) + " at time " + str(test[0].time)
    #~ print "Neuron number " + str(test[1].address.value()) + " at time " + str(test[1].time)
    #~ print "Neuron number " + str(test[2].address.value()) + " at time " + str(test[2].time) + "\n"

    rb.start_tdi[registertype] = False #reset the full flag TWO (!) times
    HICANN.set_repeater_block(h, block, rb)
    HICANN.set_repeater_block(h, block, rb)

    #set repeater mode to IDLE to prevent conflicts
    hr.setIdle()
    HICANN.set_repeater(h, Coordinate.HLineOnHICANN(hline_number).repeater(), hr)
    HICANN.flush(h) #flush to hardware

    return times, addresses


# connects senderline (6, 14, 22, 30, 38, 46, 54, 62) to receiver possibility (0-7) on a HICANN and returns according repeater/VLine number
def SetL1Crossbar(h, senderline, receiver):
    if (not(senderline == 62) and not(senderline == 54) and not(senderline == 46) and not(senderline == 38) and not(senderline == 30) and not(senderline == 22) and not(senderline == 14) and not(senderline == 6)):
        print "There is no sending repeater on this line!"
    repblock = 4
    repnr = 0

    if (receiver < 4): #left side
        repnr = 32*receiver + (62-senderline)/2
        SetLeftCrossbarLine(h, repnr, senderline)
    else:              #right side
        repnr = 31 + 32*receiver - (62-senderline)/2
        SetRightCrossbarLine(h, repnr, senderline)
        repblock = 5

    return repblock, repnr
