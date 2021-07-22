#!/usr/bin/python

import os, sys, json, copy
from itertools import chain, groupby
from numpy import array
from lxml import etree
from pyhalco_hicann_v2 import FPGAGlobal, HICANNGlobal, NeuronOnHICANN, NeuronGlobal, FGBlockOnHICANN, QuadOnHICANN, NeuronOnQuad
from pyhalbe.HICANN import neuron_parameter, shared_parameter, FGBlock
from pyhalco_common import Enum, top, bottom, left, right

emptyjson = {
    'voltages': {'neuron':{},
                'synapses0':{},
                'synapses1':{}
                },
    'currents': {'neuron':{}}
}


somedefault_values = {
	'voltages': {
		'neuron':  {
			#'Vbb':           { 'width': 1, 'values': [ 1.8 ] },
			'vdda':          { 'width': 1, 'values': [ 1.8 ] },
			#'Viresetglobal': { 'width': 2, 'values': [ 1.8, 1.8 ] },
			#'memi':          { 'width': 8, 'values': [ 1.8, 1.8, 1.8, 0.0, 1.8, 0.0, 0.0, 1.8 ] },
			#'Vsynx':         { 'width': 2, 'values': [ 0.9991202346041056, 0.8991202346041056 ] },
			#'Vreset':        { 'width': 2, 'values': [ 0.3278592375366569, 0.3278592375366569 ] },
			#'slow':          { 'width': 3, 'values': [ 1.8, 0.0, 0.0 ] },
			#'Vbexpb':        { 'width': 2, 'values': [ 1.8, 1.8 ] },
			#'Vsyni':         { 'width': 2, 'values': [ 0.8991202346041056, 0.8991202346041056 ] },
			'reset':         { 'width': 1, 'values': [ 0.0 ] },
			#'El':            { 'width': 2, 'values': [ 0.4398826979472141, 0.4398826979472141 ] },
			#'Vsyntci':       { 'width': 2, 'values': [ 1.4076246334310851, 1.4076246334310851 ] },
			#'Esynx':         { 'width': 2, 'values': [ 1.002932551319648, 1.002932551319648 ] },
			'writeMem':      { 'width': 1, 'values': [ 1.8 ] },
			#'fast':          { 'width': 3, 'values': [ 1.8, 0.0, 0.0 ] },
			#'Vsyntcx':       { 'width': 2, 'values': [ 1.3407, 1.3407 ] },
			#'Vexp':          { 'width': 2, 'values': [ 0.7038123167155426, 0.7038123167155426 ] },
			#'Vt':            { 'width': 2, 'values': [ 1.0557184750733137, 1.0557184750733137 ] },
			'vdd':           { 'width': 1, 'values': [ 1.8 ] },
			#'Esyni':         { 'width': 2, 'values': [ 0.17595307917888564, 0.17595307917888564 ] },
			#'bigcap':        { 'width': 1, 'values': [ 1.8 ] },
			#'bigcap_n':      { 'width': 1, 'values': [ 0.0 ] },
			#'memib':         { 'width': 8, 'values': [ -0.0, -0.0, -0.0, 1.8, -0.0, 1.8, 1.8, -0.0 ] }
		},
		'synapses0': {
			'vdd':           { 'width': 1, 'values': [ 1.8 ] },
			'w':             { 'width': 2, 'values': [[10,0.0,10,1.8,10,0.0], [10,0.0,10,0.0,10,1.8]]},
			#'d':             { 'width': 4, 'values': [[10,0.0,10,1.8,10,1.8], [10,0.0,10,1.8,10,1.8], [10,0.0,10,1.8,10,1.8], [10,0.0,10,0.0,10,0.0]]},
			#'db':            { 'width': 4, 'values': [[10,1.8,10,0.0,10,0.0], [10,1.8,10,0.0,10,0.0], [10,1.8,10,0.0,10,0.0], [10,1.8,10,1.8,10,1.8]]},
			'adr':           { 'width': 4, 'values': [ 0.0, 0.0, 0.0, 0.0 ] },
			'adrb':          { 'width': 4, 'values': [ 1.8, 1.8, 1.8, 1.8 ] },
			#'pre':           { 'width': 2, 'values': [ [ 100,0.0, 1,1.8, 100,0.0, 1,1.8 ], 0.0 ] },
			'wdec':          { 'width': 2, 'values': [[10,1.8,10,0.0,10,0.0], [10,1.8,10,0.0,10,0.0]] },
			'post':          { 'width': 1, 'values': [ 0.0 ] }
		},
		'synapses1': {
			'vdd':           { 'width': 1, 'values': [ 1.8 ] },
			'w':             { 'width': 2, 'values': [[10,0.0,10,1.8,10,0.0], [10,0.0,10,0.0,10,1.8]]},
			#'d':             { 'width': 4, 'values': [[10,0.0,10,1.8,10,1.8], [10,0.0,10,1.8,10,1.8], [10,0.0,10,1.8,10,1.8], [10,0.0,10,0.0,10,0.0]]},
			#'db':            { 'width': 4, 'values': [[10,1.8,10,0.0,10,0.0], [10,1.8,10,0.0,10,0.0], [10,1.8,10,0.0,10,0.0], [10,1.8,10,1.8,10,1.8]]},
			'adr':           { 'width': 4, 'values': [ 0.0, 0.0, 0.0, 0.0 ] },
			'adrb':          { 'width': 4, 'values': [ 1.8, 1.8, 1.8, 1.8 ] },
			#'pre':           { 'width': 2, 'values': [ [ 100,0.0, 1,1.8, 100,0.0, 1,1.8 ], 0.0 ] },
			'wdec':          { 'width': 2, 'values': [[10,1.8,10,0.0,10,0.0], [10,1.8,10,0.0,10,0.0]] },
			'post':          { 'width': 1, 'values': [ 0.0 ] }
		},
		'syndrv_synctrl': {
			'vdd':           { 'width': 1, 'values': [ 1.8 ] },
			'vdda':          { 'width': 1, 'values': [ 1.8 ] },
			'Vgmax':         { 'width': 4, 'values': [ 0.0, 0.0, 0.0, 0.0 ] },
			'en':            { 'width': 1, 'values': [ 1.8 ] },
			'a':             { 'width': 7, 'values': [ 1.8, 1.8, 1.8, 1.8, 1.8, 1.8, 1.8 ] },
			'enctrlb':       { 'width': 1, 'values': [0.0] }
		},
		'syndrv_synpart': {
			'vdd':           { 'width': 1, 'values': [ 1.8 ] },
			'vdda':          { 'width': 1, 'values': [ 1.8 ] },
			'd':             { 'width': 8, 'values': [ 1.8, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 ] },
			'db':            { 'width': 8, 'values': [ 0.0, 1.8, 1.8, 1.8, 1.8, 1.8, 1.8, 1.8 ] }
		}
	},

	'currents': {
		'neuron': {
			#'currentin':     { 'width': 2, 'values': [ 0.0, 0.0 ] },
			#'Ispikeamp':     { 'width': 2, 'values': [ 2.5e-06, 2.5e-06 ] },
			#'Ifireb':        { 'width': 2, 'values': [ 1.2487781036168135e-06, 1.2487781036168135e-06 ] },
			#'Iconvx':        { 'width': 2, 'values': [ 1.0e-6, 1.0e-6 ] },
			#'Ipl':           { 'width': 2, 'values': [ 1.2487781036168135e-06, 1.2487781036168135e-06 ] },
			#'Igladapt':      { 'width': 2, 'values': [ 2.443792766373412e-10, 2.443792766373412e-10 ] },
			#'Irexp':         { 'width': 2, 'values': [ 0.0, 0.0 ] },
			#'Iradapt':       { 'width': 2, 'values': [ 1.2487781036168135e-06, 1.2487781036168135e-06 ] },
			#'Iintbbi':       { 'width': 2, 'values': [ 1.2487781036168135e-06, 1.2487781036168135e-06 ] },
			#'Ibexpb':        { 'width': 2, 'values': [ 0.0, 0.0 ] },
			#'Iconvi':        { 'width': 2, 'values': [ 0.1e-9, 0.1e-9 ] },
			#'Igl':           { 'width': 2, 'values': [ 1.221896383186706e-06, 1.221896383186706e-06 ] },
			#'Iintbbx':       { 'width': 2, 'values': [ 1.2487781036168135e-06, 1.2487781036168135e-06 ] }
		},
		'synapses0':         { },
		'synapses1':         { },
		'syndrv_synpart':    { },
		'syndrv_synctrl':    { }
	},

    # Andi, holy ugliness!
	'initials': {
		'neuron': { },
		'synapses0': { },
		'synapses1': { },
		'syndrv_synctrl': { },
		'syndrv_synpart': { }
	}
}

def fg_transform_invert(value):
    return 1023-value

def fg_transform_identity(value):
    return value

fg_name_lut = {
    'shared': {
        # name -> [ volOrCur, width (left/right denmem?), left/right fg block, halbe parameter ]
        'Vreset':        [ shared_parameter.V_reset,  left,  'voltages', 2 ],
        'Vbexpb':        [ shared_parameter.V_bexp,   right, 'voltages', 2 ],
        'Viresetglobal': [ shared_parameter.I_breset, left,  'voltages', 2 ],
        'Vbb':           [ shared_parameter.V_bout,   left,  'voltages', 1 ],
    },
    'neuron': {
        'Iradapt':       [ neuron_parameter.I_radapt,   'currents', 2],
        'Vsyni':         [ neuron_parameter.V_syni,     'voltages', 2],
        'Vexp':          [ neuron_parameter.V_exp,      'voltages', 2],
        'Ispikeamp':     [ neuron_parameter.I_spikeamp, 'currents', 2],
        'Irexp':         [ neuron_parameter.I_rexp,     'currents', 2],
        'Iintbbx':       [ neuron_parameter.I_intbbx,   'currents', 2],
        'Esynx':         [ neuron_parameter.E_synx,     'voltages', 2],
        'Vsyntcx':       [ neuron_parameter.V_syntcx,   'voltages', 2],
        'El':            [ neuron_parameter.E_l,        'voltages', 2],
        'Iconvi':        [ neuron_parameter.I_convi,    'currents', 2],
        'Vsynx':         [ neuron_parameter.V_synx,     'voltages', 2],
        'Ipl':           [ neuron_parameter.I_pl,       'currents', 2],
        'Igladapt':      [ neuron_parameter.I_gladapt,  'currents', 2],
        'Igl':           [ neuron_parameter.I_gl,       'currents', 2],
        'Iintbbi':       [ neuron_parameter.I_intbbi,   'currents', 2],
        'Vsyntci':       [ neuron_parameter.V_syntci,   'voltages', 2],
        'Esyni':         [ neuron_parameter.E_syni,     'voltages', 2],
        'Vt':            [ neuron_parameter.V_t,        'voltages', 2],
        'Iconvx':        [ neuron_parameter.I_convx,    'currents', 2],
        'Ibexpb':        [ neuron_parameter.I_bexp,     'currents', 2],
        'Ifireb':        [ neuron_parameter.I_fire,     'currents', 2],
    }
}

neuron_config = [
    # multiply by third element
    { 'dest' : 'bigcap',   'invert' : True , 'halbe_names' : ['bigcap']},
    { 'dest' : 'bigcap_n', 'invert' : False, 'halbe_names' : ['bigcap']},
    { 'dest' : 'slow',     'invert' : True , 'halbe_names' : ['slow_I_gl', 'slow_I_radapt', 'slow_I_gladapt']}, #True
    { 'dest' : 'fast',     'invert' : True , 'halbe_names' : ['fast_I_gl', 'fast_I_radapt', 'fast_I_gladapt']} #True
]


voltage_true = {
    True:  1.8,
    False: 0.0
}

invert_voltage = {
	1.8: 0.0,
	0.0: 1.8
}

bits_to_value = {
    '1':  1.8,
    '0': 0.0
}


def merge(a, b, path = []):
    '''merges b into a'''
    #if path is None: path = []
    #print path, b.keys()
    for key in b:
        #print path + [key], type(b[key])
        if key in a:
            if isinstance(a[key], dict) and isinstance(b[key], dict):
                #print "merge it", path + [key]
                merge(a[key], b[key], path + [str(key)])
            #elif a[key] == b[key]:
            #    print path + [key], "samesame"
            #    pass # same leaf value
            else:
                raise Exception('Conflict at %s' % '.'.join(path + [str(key)]))
        else:
            #print "setting", path + [key], "to", b[key]
            a[key] = b[key]
    return a


class HALbeXML2Sim2Denmem(object):
    """@brief formats a given xml-formatted dump file for the denmem circuit simulation

    @param xmlfile path to input file
    @param neuron  NeuronGlobal coordinate
    """


    def __init__(self, xmlfile, coordinate):
        self.xmlfile = xmlfile
        self.coordinate = coordinate

        if self.coordinate.toNeuronOnQuad().x() != left:
            raise Exception("Simulator supports only left denmem")

        # find (horizontally) neighboring denmem :)
        self.neighbor = None
        if coordinate.toNeuronOnQuad().x() == left:
            self.neighbor = NeuronOnHICANN(coordinate.toQuadrantOnHICANN(), NeuronOnQuad(right, coordinate.toNeuronOnQuad().y()))
        #elif coordinate.toNeuronOnQuad().x() == right:
        #    self.neighbor = NeuronOnHICANN(coordinate.toQuadrantOnHICANN(), NeuronOnQuad(left, coordinate.toNeuronOnQuad().y()))
        else:
            raise Exception("WARG")
        self.neighbor = NeuronGlobal(self.neighbor, self.coordinate.hicann())
        self.neighbor_fgblock = FGBlockOnHICANN(Enum(int(self.coordinate.toNeuronFGBlock().toEnum())+1))

        self.tree = etree.parse(self.xmlfile)

        # storage of parameters -> to be used for transformation
        self.out = dict()

        # output: denmem simulator compatible data dictionary
        #self.json_out = {
        #    'voltages': {
        #        'neuron': {},
        #        'synapses0': {},
        #        'synapses1': {},
        #        'syndrv_synpart': {},
        #        'syndrv_synctrl': {}
        #    },
        #    'currents': {
        #        'neuron': {},
        #        'synapses0': {},
        #        'synapses1': {},
        #        'syndrv_synpart': {},
        #        'syndrv_synctrl': {}
        #    }
        #}
        self.json_out = copy.deepcopy(somedefault_values)

    def extract(self):
        self.extract_fg_values()
        self.extract_denmem_quad()
        self.extract_neuron_config()
        self.extract_current_stimulus()
        self.extract_playback_pulses()
        self.extract_synapse_weights()
        for elem in ['fg_shared', 'fg_neuron', 'denmem_quad', 'neuron_config']:
            if not elem in list(self.out.keys()):
                raise Exception("Data missing: %s" % elem)
        self.extracted = True


    def write(self, outfile):
        if not getattr(self, 'extracted', None):
            self.extract()
        #print "wiring to", outfile
        with open(outfile, 'w') as outfile:
            json.dump(self.json_out, outfile, indent=4)


    ########################
    # helpers              #
    ########################
    def recursive_dict_iter(self, element):
        # filter out extranous <TAG><value>X</value></TAG> things
        if len(element.getchildren()) == 1 and element.getchildren()[0].tag == 'value':
            return element.tag, element.getchildren()[0].text
        return element.tag, dict(list(map(self.recursive_dict_iter, element))) or element.text


    def recursive_dict(self, element):
        return self.recursive_dict_iter(element)[1]


    def get_parameters(self, node):
        all = []
        n = node.getnext()
        while((not n is None) and (n.tag != 'type')):
            all.append(n)
            n = n.getnext()
        hicann, parameters = all[0], all[1:]
        return (hicann, parameters)

    def values_to_int(self, d):
        if isinstance(d, dict):
            return dict([(x[0], int(x[1])) for x in list(d.items())])
        # else try plain list-style
        return list(map(int, d))


    def get_all_functions(self, foo, fpga=False):
        all = self.tree.xpath('/boost_serialization/type[text()="%s"]' % foo)
        for n in all:
            hicann, parameters = self.get_parameters(n)
            if not fpga:
                hc = HICANNGlobal(Enum(self.values_to_int(self.recursive_dict(hicann))['e']))
            else:
                hc = HICANNGlobal(Enum(self.values_to_int(self.recursive_dict(hicann))['wafer'])) #or 'value'
            if self.coordinate.hicann() != hc and not fpga:
                print("ignore non-matching hicanns...")
                print("ignoring HICANN", hc, "(looking for", self.coordinate.hicann(), ")")
                continue
            else:
                yield hicann, parameters


    def dac_to(self, const, v):
        #conv = lambda x: const * int(x)
        def conv(x):
            assert (isinstance(x, int))
            return const * int(x)
        try:
            iter(v)
            ret = list(map(conv, v))
        except TypeError:
            ret = conv(v)
        return ret

    dac_to_volt_const = 1.8/1023
    dac_to_current_const = 0.0000025/1023 # TODO: by MK -- correct?
    dac_to_current = lambda self, v: self.dac_to(self.dac_to_current_const, v)
    dac_to_volt = lambda self, v: self.dac_to(self.dac_to_volt_const, v)


    ########################
    # floating gate values #
    ########################
    def extract_fg_values(self):
        ncolumns = 24
        nrows = 128
        if 'fg_shared' not in self.out:
            self.out['fg_shared'] = [None, None]

        for hicann, parameters in self.get_all_functions('set_fg_values'):

            assert len(parameters) in [1, 2]
            fgc = None
            fgb_left = None
            fgb_right = None

            # FGCtrl-based overload
            if len(parameters) == 1:
                fgc = self.coordinate.toNeuronFGBlock()
                fgb_left = parameters[0].xpath('blocks/elems/item')[int(fgc.toEnum())]
                fgb_right = parameters[0].xpath('blocks/elems/item')[int(self.neighbor_fgblock.toEnum())]

            # FGBlockOnHICANN + FGBlock data overload
            elif len(parameters) == 2:
                fgc = FGBlockOnHICANN(Enum(int(self.recursive_dict(parameters[0])['e'])))
                if fgc == self.coordinate.toNeuronFGBlock():
                    fgb_left = parameters[1]
                elif fgc == self.neighbor_fgblock:
                    fgb_right = parameters[1]
                else:
                    continue

            shared_left = None
            neuron = None

            if fgb_left is not None: # neuron is on the left :)
                shared_left = array([int(x) for x in fgb_left.xpath('shared/elems/item/value/text()')]).reshape(ncolumns)
                neuron = array([int(x) for x in fgb_left.xpath('neuron/elems/item/elems/item/value/text()')]).reshape((nrows, ncolumns))
            if fgb_right is not None:
                shared_right = array([int(x) for x in fgb_right.xpath('shared/elems/item/value/text()')]).reshape(ncolumns)

            if shared_left is not None:
                self.out['fg_shared'][int(left)] = [int(x) for x in shared_left]
            if shared_right is not None:
                self.out['fg_shared'][int(right)] = [int(x) for x in shared_right]
            self.out['fg_neuron'] = [ self.values_to_int(neuron[self.coordinate.toNeuronOnFGBlock()]), # left
                                      self.values_to_int(neuron[self.neighbor.toNeuronOnFGBlock()]) ]  # right

        mydictupdates = copy.deepcopy(emptyjson)

        # translate shared FG parameters
        for key, data in list(fg_name_lut['shared'].items()):
            HALbeType   = data[0]
            leftOrRight = data[1]
            sectionName = data[2]
            valueWidth  = data[3]
            if leftOrRight == left:
                mydictupdates[sectionName]['neuron'][key] = {
                    'width': valueWidth,
                    'values': valueWidth * [self.out['fg_shared'][int(leftOrRight)][FGBlock.getSharedHardwareIdx(self.coordinate.toNeuronFGBlock(), HALbeType)]]
                }
            else:
                mydictupdates[sectionName]['neuron'][key] = {
                    'width': valueWidth,
                    'values': valueWidth * [self.out['fg_shared'][int(leftOrRight)][FGBlock.getSharedHardwareIdx(self.neighbor_fgblock, HALbeType)]]
                }

        # translate neuron FG parameters
        for key, data in list(fg_name_lut['neuron'].items()):
            HALbeType   = data[0]
            sectionName = data[1]
            valueWidth  = data[2]
            
            mydictupdates[sectionName]['neuron'][key] = {
                'width': valueWidth,
                'values': [ self.out['fg_neuron'][int(left) ][FGBlock.getNeuronHardwareIdx(self.coordinate.toNeuronFGBlock(), HALbeType)],
                            self.out['fg_neuron'][int(right)][FGBlock.getNeuronHardwareIdx(self.coordinate.toNeuronFGBlock(), HALbeType)] ]
            }

            #if str(key) == 'Vsyni':
                #print "jap"
                #print mydictupdates

        # convert FG DAC values to voltages/currents
        for k, v in list(mydictupdates['voltages'].items()):
            for k2, v2 in list(v.items()):
                v2['values'] = self.dac_to_volt(v2['values'])
        for k, v in list(mydictupdates['currents'].items()):
            for k2, v2 in list(v.items()):
                v2['values'] = self.dac_to_current(v2['values'])

        #print 'zzz', mydictupdates
        #print mydictupdates['voltages']['neuron'].keys()
        merge(self.json_out, mydictupdates)


    ########################
    # denmem quad          #
    ########################
    def extract_denmem_quad(self):
        for hicann, parameters in self.get_all_functions('set_denmem_quad'):

            assert len(parameters) == 2

            qc, parameter = parameters
            qc = QuadOnHICANN(int(self.recursive_dict(qc)))

            if qc != self.coordinate.toQuadrantOnHICANN():
                continue

            hbits = parameter.xpath('horizontal/bits/text()')
            vbits = parameter.xpath('vertical/bits/text()')
            assert len(hbits) == 1 and len(vbits) == 1

            cfgs = [self.recursive_dict(x) for x in parameter.xpath('neuron/elems/item/elems/item')]
            assert len(cfgs) == 4

            left_quad  = int(self.coordinate.toNeuronOnQuad().toEnum())
            right_quad = int(self.coordinate.toNeuronOnQuad().toEnum())+1
            self.out['denmem_quad'] = {
                'horizontal': hbits[0],
                'vertical': vbits[0],
                'cfg': [cfgs[left_quad], cfgs[right_quad]]
            }

        # translate denmem config (ugly!)
        val = 1*int(self.out['denmem_quad']['cfg'][int(right)]['enable_aout'])          + \
              2*int(self.out['denmem_quad']['cfg'][int(left)]['enable_aout'])           + \
              4*int(self.out['denmem_quad']['cfg'][int(right)]['enable_current_input']) + \
              8*int(self.out['denmem_quad']['cfg'][int(left)]['enable_current_input'])
        assert val < 6 or val == 8 or val == 10
        if val == 8: val = 7
        elif val == 10: val = 6

        memi_values = [
            1.8 * ((val & 1) >> 0),
            1.8 * ((val & 2) >> 1),
            1.8 * ((val & 4) >> 2),
            1.8 * int(self.out['denmem_quad']['horizontal'][1]), # only top, high bit!
            1.8 * int(self.out['denmem_quad']['cfg'][int(left)]['activate_firing']),
            1.8 * int(self.out['denmem_quad']['cfg'][int(left)]['enable_fire_input']),
            1.8 * int(self.out['denmem_quad']['cfg'][int(right)]['enable_fire_input']),
            1.8 * int(self.out['denmem_quad']['cfg'][int(right)]['activate_firing']),
        ]

        mydictupdates = copy.deepcopy(emptyjson)

        mydictupdates['voltages']['neuron']['memi'] = {
            'width': 8,
            'values': memi_values
        }

        mydictupdates['voltages']['neuron']['memib'] = {
            'width': 8,
            'values': [-(x - 1.8) for x in memi_values]
        }

        merge(self.json_out, mydictupdates)




    #########################
    ## neuron config        #
    #########################
    def extract_neuron_config(self):
        mydictupdates = copy.deepcopy(emptyjson)

        for hicann, parameters in self.get_all_functions('set_neuron_config'):
            assert len(parameters) == 1
            neuron_config_list = []
            # only "top" used by simulation => higher bit => 1
            for (k,v) in list(self.recursive_dict(parameters[0]).items()):
                try:
                    neuron_config_list.append([k, bits_to_value[v['bits'][1]]])
                except TypeError:
                    neuron_config_list.append([k, bits_to_value[v]])

            self.out['neuron_config'] = dict(neuron_config_list)

        # translate neuron config
        for elem in neuron_config:
            target = mydictupdates['voltages']['neuron'][elem['dest']] = {}
            target['width'] = len(elem['halbe_names'])
            if elem['invert']:
                target['values'] = [invert_voltage[self.out['neuron_config'][elem['halbe_names'][x]]] for x in range(len(elem['halbe_names']))]
            else:
                target['values'] = [self.out['neuron_config'][elem['halbe_names'][x]] for x in range(len(elem['halbe_names']))]

        merge(self.json_out, mydictupdates)


    #########################
    ## current stimulus     #
    #########################
    def extract_current_stimulus(self, wait_cycles=100):
        self.out['stimulus'] = [0.0, 0.0] # set defaults
        for hicann, parameters in self.get_all_functions('set_current_stimulus'):
            assert len(parameters) == 2
            values = list(map(int, parameters[1].xpath('current/elems/item/text()')))
            pulselength = int(parameters[1].xpath('pulselength/value/text()')[0])
            continuous = int(parameters[1].xpath('continuous/text()')[0])

            compress = lambda l: list(chain(*[[pulselength*len(list(g)), v] for v, g in groupby(l)]))
            values = compress(list(map(self.dac_to_current, values)))
            values = [wait_cycles, 0.0] + values # wait some clock cycles until simulator has reached E_l

            if int(self.out['denmem_quad']['cfg'][int(left)]['enable_current_input']) != 0:
                self.out['stimulus'] = [values, 0.0]
            elif int(self.out['denmem_quad']['cfg'][int(right)]['enable_current_input']) != 0:
                self.out['stimulus'] = [0.0, values]
            else:
                self.out['stimulus'] = [0.0, 0.0]


        mydictupdates = copy.deepcopy(emptyjson)
        mydictupdates['currents']['neuron']['currentin'] = {
            'width': 2,
            'values': self.out['stimulus']
        }
        merge(self.json_out, mydictupdates)


    #########################
    ## playback pulses     ##
    #########################       
    def extract_playback_pulses(self):
        mydictupdates = copy.deepcopy(emptyjson)
        self.out['pulses'] = {}
        self.out['pulses']['448'] = [100,0.0, 1,1.8, 100,0.0, 1,1.8] #set defaults #left top
        self.out['pulses']['449'] = 0.0 							#set defaults #left bottom
        self.out['pulses']['450'] = [100,0.0, 1,1.8, 100,0.0, 1,1.8] #set defaults #right top
        self.out['pulses']['451'] = 0.0 							#set defaults #right bottom

        for fpga, parameters in self.get_all_functions('write_playback_pulses', True):
            assert len(parameters) == 2
            pulse_count = int(parameters[0].xpath('pulses/count/text()')[0])
            pulse_time = list(map(int, parameters[0].xpath('pulses/item/time/text()')))
            pulse_address = list(map(int, parameters[0].xpath('pulses/item/pulse_address/label/text()')))
            delay = int(parameters[1].xpath('text()')[0]) + pulse_time[0]
            pulse_length = 1
            
            self.out['pulses'][str(pulse_address[0])] = [delay, 0.0]
            
            for i in range(len(pulse_time)):
               self.out['pulses'][str(pulse_address[0])].append(pulse_length)
               self.out['pulses'][str(pulse_address[0])].append(1.8)
               self.out['pulses'][str(pulse_address[0])].append(int(pulse_time[1] - pulse_time[0] - pulse_length))
               self.out['pulses'][str(pulse_address[0])].append(0.0)
               
        mydictupdates['voltages']['synapses0']['pre'] = {
            'width': 2,
            'values': [self.out['pulses']['448'], self.out['pulses']['449']]
            }
        mydictupdates['voltages']['synapses1']['pre'] = {
            'width': 2,
            'values': [self.out['pulses']['450'], self.out['pulses']['451']]
            }

        merge(self.json_out, mydictupdates)
        
        
    def extract_synapse_weights(self):
        self.out['syn_weights0'] = {
            'd': [[10,0.0,10,1.8,10,1.8], [10,0.0,10,1.8,10,1.8], [10,0.0,10,1.8,10,1.8], [10,0.0,10,0.0,10,0.0]], #set d and db stuff
            'db': [[10,1.8,10,0.0,10,0.0], [10,1.8,10,0.0,10,0.0], [10,1.8,10,0.0,10,0.0], [10,1.8,10,1.8,10,1.8]],
        }
        self.out['syn_weights1'] = {
            'd': [[10,0.0,10,1.8,10,1.8], [10,0.0,10,1.8,10,1.8], [10,0.0,10,1.8,10,1.8], [10,0.0,10,0.0,10,0.0]],
            'db': [[10,1.8,10,0.0,10,0.0], [10,1.8,10,0.0,10,0.0], [10,1.8,10,0.0,10,0.0], [10,1.8,10,1.8,10,1.8]],
        }
        
        self.out['bits_0_0'] = [1.8, 1.8, 1.8, 1.8] #set defaults, full
        self.out['bits_0_1'] = [1.8, 1.8, 1.8, 1.8]
        self.out['bits_1_0'] = [1.8, 1.8, 1.8, 1.8]
        self.out['bits_1_1'] = [1.8, 1.8, 1.8, 1.8]
        
        mydictupdates = copy.deepcopy(emptyjson)
        run = 1 #bad bad hotfix by david
        for hicann, parameters in self.get_all_functions('set_weights_row'):
           # print hicann, parameters
            run += 2
            syn_address = int(parameters[0].xpath('value/text()')[0])
            rows_count = int(parameters[1].xpath('elems/count/text()')[0])
            weight_row = list(map(int, parameters[1].xpath('elems/item/value/text()')))

            for i in range (2):
                weights = [
                    1.8 * ((weight_row[i] & 1) >> 0),
                    1.8 * ((weight_row[i] & 2) >> 1),
                    1.8 * ((weight_row[i] & 4) >> 2),
                    1.8 * ((weight_row[i] & 8) >> 3)
                ]
                for j in range (4):
                    self.out['syn_weights'+str(i)]['d'][j][run] = weights[j]                
                    self.out['syn_weights'+str(i)]['db'][j][run] = invert_voltage[weights[j]]


      #  self.out['syn_weights0']


        for i in range (2):    
            mydictupdates['voltages']['synapses'+str(i)]['d'] = {
                'width': 4,
                'values': self.out['syn_weights'+str(i)]['d']
            }
            mydictupdates['voltages']['synapses'+str(i)]['db'] = {
                'width': 4,
                'values': self.out['syn_weights'+str(i)]['db']
            }
                        
        merge(self.json_out, mydictupdates)


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--input-file', help='Specify HALbe XML input file')
    parser.add_argument('-o', '--output-file', help='Specify SimDenmem output file')
    parser.add_argument('-N', '--neuron', default=0, type=int, help='Specify NeuronOnHICANN for simulation')
    parser.add_argument('-H', '--hicann', default=216, type=int, help='Specify HICANNGlobal for simulation')
    args = parser.parse_args()

    inf  = args.input_file
    outf = args.output_file
    nrn  = args.neuron

    # non-empty source file
    assert os.path.exists(inf) and os.stat(inf).st_size != 0
    # empty target file
    #assert not os.path.exists(outf) or os.stat(outf).st_size == 0

    ng = NeuronGlobal(NeuronOnHICANN(Enum(args.neuron)), HICANNGlobal(Enum(args.hicann)))

    sim = HALbeXML2Sim2Denmem(inf, ng)
    sim.extract()
    sim.write(outf)
